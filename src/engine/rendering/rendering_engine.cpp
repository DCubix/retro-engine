#include "rendering_engine.h"

#include <fmt/format.h>

#include "shaders/generated/phong_vert.h"
#include "shaders/generated/phong_frag.h"

#include "shaders/generated/gbuffer_vert.h"
#include "shaders/generated/gbuffer_frag.h"

#include "shaders/generated/lighting_v2_frag.h"

#include "shaders/generated/directional_shadows_vert.h"
#include "shaders/generated/shadows_frag.h"

#include "shaders/generated/point_shadows_vert.h"
#include "shaders/generated/point_shadows_frag.h"
#include "shaders/generated/point_shadows_geom.h"

#include "shaders/generated/quad_vert.h"
#include "shaders/generated/quad_frag.h"

#include "../utils/logger.h"

RenderingEngine::RenderingEngine(u32 framebufferWidth, u32 framebufferHeight)
{
	const float aspectRatio = static_cast<float>(framebufferWidth) / static_cast<float>(framebufferHeight);

	mDebugDraw = std::make_unique<DebugDraw>(framebufferWidth, framebufferHeight);

	mProjectionMatrix = MatrixPerspective(
		45.0f * DEG2RAD, // FOV
		aspectRatio, // Aspect ratio
		0.1f, // Near plane
		500.0f // Far plane
	);
	mViewMatrix = MatrixTranslate(0.0f, 0.0f, -4.0f);

	mTexturesSSBO = Buffer<GLuint64, BufferType::shaderStorageBuffer>(BufferUsage::dynamicDraw);
	mTexturesSSBO.BindBase(0);

	mMaterialsSSBO = Buffer<MaterialShaderData, BufferType::shaderStorageBuffer>(BufferUsage::dynamicDraw);
	mMaterialsSSBO.BindBase(1);

	mLightUBO = Buffer<LightShaderData, BufferType::uniformBuffer>(BufferUsage::dynamicDraw);
	mLightUBO.BindBase(2);

	mLightMatricesUBO = Buffer<Matrix, BufferType::uniformBuffer>(BufferUsage::dynamicDraw);
	mLightMatricesUBO.BindBase(3);

	{
		ShaderDescription shaderDesc{
			.shaderSources = {
				{ ShaderType::vertexShader, quad_vert_src },
				{ ShaderType::fragmentShader, quad_frag_src }
			},
		};
		mQuadShader = std::make_unique<Shader>(shaderDesc);
	}

	{ // GBuffer setup
		FrameBufferDescription frameBufferDesc{};
		frameBufferDesc.width = framebufferWidth;
		frameBufferDesc.height = framebufferHeight;
		frameBufferDesc.colorAttachments = {
			TextureFormat::RGBA,       // diffuse, roughness
			TextureFormat::RGBA,       // emissive, metallic
			TextureFormat::RGBf,       // position
			TextureFormat::RGBf,       // normal
		};
		frameBufferDesc.depth.enabled = true;
		frameBufferDesc.renderBuffer.enabled = true;
		frameBufferDesc.renderBuffer.format = RenderBufferFormat::depth;

		mGBuffer = std::make_unique<FrameBuffer>(frameBufferDesc);

		PassDescription passDesc{};
		passDesc.clearColor = { 0.0f, 0.0f, 0.0f, 0.0f };
		passDesc.clear.color = true;
		passDesc.clear.depth = true;
		passDesc.frameBuffer = mGBuffer.get();

		mGBufferPass = Pass(passDesc);

		ShaderDescription shaderDesc{
			.shaderSources = {
				{ ShaderType::vertexShader, gbuffer_vert_src },
				{ ShaderType::fragmentShader, gbuffer_frag_src }
			},
		};
		mGBufferShader = std::make_unique<Shader>(shaderDesc);

		PipelineDescription pipelineDesc{};
		pipelineDesc.depthStencilState.depth.enabled = true;
		pipelineDesc.blendState.enabled = false;
		pipelineDesc.rasterizerState.geometry.windingOrder = WindingOrder::counterClockwise;
		pipelineDesc.rasterizerState.geometry.cullMode = CullMode::back;

		mGBufferPipeline = Pipeline(pipelineDesc);
	}

	{ // Lighting setup
		FrameBufferDescription frameBufferDesc{};
		frameBufferDesc.width = framebufferWidth;
		frameBufferDesc.height = framebufferHeight;
		frameBufferDesc.colorAttachments = {
			TextureFormat::RGBf,
		};

		mLightingBuffer = std::make_unique<FrameBuffer>(frameBufferDesc);

		PassDescription passDesc{};
		passDesc.clearColor = { 0.0f, 0.0f, 0.0f, 0.0f };
		passDesc.clear.color = true;
		passDesc.frameBuffer = mLightingBuffer.get();

		mLightingPass = Pass(passDesc);

		ShaderDescription shaderDesc{
			.shaderSources = {
				{ ShaderType::vertexShader, quad_vert_src },
				{ ShaderType::fragmentShader, lighting_v2_frag_src }
			},
		};
		mLightingShader = std::make_unique<Shader>(shaderDesc);

		PipelineDescription pipelineDesc{};
		pipelineDesc.shader = mLightingShader.get();

		mLightingPipeline = Pipeline(pipelineDesc);
	}

	{ // Post processing setup
		FrameBufferDescription frameBufferDesc{};
		frameBufferDesc.width = framebufferWidth;
		frameBufferDesc.height = framebufferHeight;
		frameBufferDesc.colorAttachments = {
			TextureFormat::RGBf,
			TextureFormat::RGBf,
		};

		mPingPongBuffer = std::make_unique<FrameBuffer>(frameBufferDesc);

		PassDescription passDesc{};
		passDesc.clearColor = { 0.0f, 0.0f, 0.0f, 0.0f };
		passDesc.clear.color = true;
		passDesc.frameBuffer = mPingPongBuffer.get();

		mPostProcessingPass = Pass(passDesc);
	}

	{ // Directional Shadow setup
		FrameBufferDescription frameBufferDesc{};
		frameBufferDesc.width = 1024;
		frameBufferDesc.height = 1024;
		frameBufferDesc.depth.enabled = true;
		frameBufferDesc.depth.withStencil = false;

		mShadowBuffer = std::make_unique<FrameBuffer>(frameBufferDesc);

		PassDescription passDesc{};
		passDesc.clear.depth = true;
		passDesc.clear.color = false;
		passDesc.frameBuffer = mShadowBuffer.get();

		mShadowPass = Pass(passDesc);

		ShaderDescription shaderDesc{
			.shaderSources = {
				{ ShaderType::vertexShader, directional_shadows_vert_src },
				{ ShaderType::fragmentShader, shadows_frag_src }
			},
		};
		mShadowShader = std::make_unique<Shader>(shaderDesc);

		PipelineDescription pipelineDesc{};
		pipelineDesc.shader = mShadowShader.get();
		pipelineDesc.depthStencilState.depth.enabled = true;
		pipelineDesc.blendState.enabled = false;
		pipelineDesc.rasterizerState.geometry.windingOrder = WindingOrder::counterClockwise;
		pipelineDesc.rasterizerState.geometry.cullMode = CullMode::back;

		mShadowPipeline = Pipeline(pipelineDesc);
	}

	{ // Omnidirectional Shadow setup
		FrameBufferDescription frameBufferDesc{};
		frameBufferDesc.width = 1024;
		frameBufferDesc.height = 1024;
		frameBufferDesc.depth.enabled = true;
		frameBufferDesc.depth.withStencil = false;
		frameBufferDesc.depth.isCubeMap = true;

		mPointShadowBuffer = std::make_unique<FrameBuffer>(frameBufferDesc);

		PassDescription passDesc{};
		passDesc.clear.depth = true;
		passDesc.clear.color = false;
		passDesc.frameBuffer = mPointShadowBuffer.get();

		mPointShadowPass = Pass(passDesc);

		ShaderDescription shaderDesc{
			.shaderSources = {
				{ ShaderType::vertexShader, point_shadows_vert_src },
				{ ShaderType::fragmentShader, point_shadows_frag_src },
				{ ShaderType::geometryShader, point_shadows_geom_src }
			},
		};
		mPointShadowShader = std::make_unique<Shader>(shaderDesc);

		PipelineDescription pipelineDesc{};
		pipelineDesc.shader = mPointShadowShader.get();
		pipelineDesc.depthStencilState.depth.enabled = true;
		pipelineDesc.blendState.enabled = false;
		pipelineDesc.rasterizerState.geometry.windingOrder = WindingOrder::counterClockwise;
		pipelineDesc.rasterizerState.geometry.cullMode = CullMode::back;

		mPointShadowPipeline = Pipeline(pipelineDesc);
	}
}

void RenderingEngine::AddToRender(Mesh* mesh, const Material& material, const Opt<Matrix>& transform) {
	if (!mesh) {
		utils::LogE("Mesh is null");
		return;
	}
	mMaterials.push_back(material);
	mDrawCalls.push_back(DrawCall{
		.mesh = mesh,
		.primitiveType = mesh->GetPrimitiveType(),
		.materialId = mMaterials.size() - 1,
		.instanceCount = mesh->GetInstanceCount(),
		.transform = transform,
	});
}

void RenderingEngine::AddToRender(const Light& light)
{
	mLights.push_back(light);
}

void RenderingEngine::AddFilter(Filter* filter)
{
	mFilters.push_back(filter);
}

void RenderingEngine::RemoveFilter(Filter* filter)
{
	auto it = std::remove(mFilters.begin(), mFilters.end(), filter);
	if (it != mFilters.end()) {
		mFilters.erase(it, mFilters.end());
	}
}

Vector3 RenderingEngine::GetViewPosition() const
{
	return mViewPosition;
}

void RenderingEngine::CameraSetup(const Matrix& projectionMatrix, const Matrix& viewMatrix)
{
	mProjectionMatrix = projectionMatrix;
	mViewMatrix = viewMatrix;
}

void RenderingEngine::Render()
{
	glClear(GL_COLOR_BUFFER_BIT);

	mProfiler.Begin("GBuffer Pass");
	RunGBufferPass();
	mProfiler.End("GBuffer Pass");

	mProfiler.Begin("Lighting Pass");
	RunLightingPass();
	mProfiler.End("Lighting Pass");

	mProfiler.Begin("Post Processing Pass");
	RunPostProcessingPass();
	mProfiler.End("Post Processing Pass");

	// Cleanup
	mDrawCalls.clear();
	mMaterials.clear();

	// Blit depth from GBuffer to default framebuffer
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	mGBuffer->BindAs(FrameBufferBindMode::read);
	mGBuffer->SetReadBuffer(0);
	glBlitFramebuffer(
		0, 0, mGBuffer->GetWidth(), mGBuffer->GetHeight(),
		0, 0, mGBuffer->GetWidth(), mGBuffer->GetHeight(),
		GL_DEPTH_BUFFER_BIT,
		GL_NEAREST
	);

	//mGBuffer->SetReadBuffer(3);
	//glBlitFramebuffer(
	//	0, 0, mGBuffer->GetWidth(), mGBuffer->GetHeight(),
	//	0, 0, mGBuffer->GetWidth(), mGBuffer->GetHeight(),
	//	GL_COLOR_BUFFER_BIT,
	//	GL_LINEAR
	//);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	/*f32 aspect = static_cast<f32>(mGBuffer->GetWidth()) / static_cast<f32>(mGBuffer->GetHeight());
	DrawQuad({ 0.0f, 0.0f }, { 0.25f / aspect, 0.25f }, mShadowBuffer->GetDepthTexture(), true, 0.01f, 200.0f);*/
}

void RenderingEngine::RunGBufferPass() {
	const Matrix identity = MatrixIdentity();

	mGBufferPass.Begin();
	mGBufferPipeline.Bind();

	// build texture and material data
	std::vector<GLuint64> textureData;
	std::vector<MaterialShaderData> materialData;
	BuildSSBOMaterialData(textureData, materialData);

	mMaterialsSSBO.Bind();
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	mMaterialsSSBO.Update(materialData.data(), materialData.size());

	mTexturesSSBO.Bind();
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	mTexturesSSBO.Update(textureData.data(), textureData.size());

	// TODO: account for instanced rendering (different pipeline)
	Shader* previousShader = nullptr;
	for (auto&& drawCall : mDrawCalls) {
		auto& material = mMaterials[drawCall.materialId];
		Shader* shader = material.customShader.expired() ?
			mGBufferShader.get() : material.customShader.lock().get();

		if (previousShader != shader) shader->Bind();

		// shader is a MaterialShader?
		if (MaterialShader* mat = dynamic_cast<MaterialShader*>(shader)) {
			mat->OnBeforeRender();
		}

		shader->SetUniformMatrix4f("uProjectionMatrix", mProjectionMatrix);
		shader->SetUniformMatrix4f("uViewMatrix", mViewMatrix);
		shader->SetUniform3f("uViewPosition", GetViewPosition());
		shader->SetUniform1u("uMaterialId", static_cast<u32>(drawCall.materialId));

		shader->SetUniformMatrix4f("uModelMatrix", drawCall.transform.value_or(identity));
		drawCall.mesh->Draw();

		previousShader = shader;
	}

	mGBufferPipeline.Unbind();
	mGBufferPass.End();

	mGBuffer->GetColorTexture(0)->GenerateMipmaps();
	mGBuffer->GetColorTexture(1)->GenerateMipmaps();
}

void RenderingEngine::RunLightingPass()
{
	mLightingPass.Begin();
	mLightingPipeline.Bind();

	mLightingShader->SetUniform3f("uAmbientColor", mAmbientColor);
	mLightingShader->SetUniform3f("uViewPosition", GetViewPosition());

	mLightingShader->SetUniform2f("uOffset", { 0.0f, 0.0f });
	mLightingShader->SetUniform2f("uScale", { 1.0f, 1.0f });

	mLightingShader->SetUniformTextureHandle(
		"ugbDiffuseRoughness",
		mGBuffer->GetColorTexture(0)->GetBindlessHandle()
	);
	mLightingShader->SetUniformTextureHandle(
		"ubgEmissiveMetallic",
		mGBuffer->GetColorTexture(1)->GetBindlessHandle()
	);
	mLightingShader->SetUniformTextureHandle(
		"ugbPosition",
		mGBuffer->GetColorTexture(2)->GetBindlessHandle()
	);
	mLightingShader->SetUniformTextureHandle(
		"ugbNormal",
		mGBuffer->GetColorTexture(3)->GetBindlessHandle()
	);

	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);

	// Ambient
	mLightingShader->SetUniform1u("uLightStep", 0);

	mQuadVAO.Bind();
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	mQuadVAO.Unbind();

	mLightingShader->SetUniformBlockBinding("LightUBO", 2);

	// Direct lighting
	mLightingShader->SetUniform1u("uLightStep", 1);

	for (const auto& light : mLights) {
		if (light.type == LightType::point) {
			RunSinglePointShadowPass(light);
		}
		else {
			RunSingleDirectionalShadowPass(light);
		}
		mLightingBuffer->Bind();
		mLightingPipeline.Bind();
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE);

		LightShaderData data{
			.colorIntensity = {
				light.color.x,
				light.color.y,
				light.color.z,
				light.intensity
			},
			.positionRadius = {
				light.position.x,
				light.position.y,
				light.position.z,
				light.radius
			},
			.directionCutoffAngle = {
				light.direction.x,
				light.direction.y,
				light.direction.z,
				light.cutoffAngle
			},
			.lightType = static_cast<i32>(light.type)
		};
		mLightUBO.Update(&data, 1);

		mLightingShader->SetUniform("uLightViewProjection", light.GetViewProjection());
		mLightingShader->SetUniform("uShadowEnabled", light.shadowsCaster);
		if (light.shadowsCaster) {
			if (light.type == LightType::point) {
				mLightingShader->SetUniformTextureHandle(
					"uShadowCubeMap",
					mPointShadowBuffer->GetDepthTexture()->GetBindlessHandle()
				);
				mLightingShader->SetUniform("uShadowFar", light.radius);
			}
			else {
				mLightingShader->SetUniformTextureHandle(
					"uShadowMap",
					mShadowBuffer->GetDepthTexture()->GetBindlessHandle()
				);
			}
		}

		mQuadVAO.Bind();
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		mQuadVAO.Unbind();
	}

	// Emissive
	mLightingShader->SetUniform1u("uLightStep", 2);
	mQuadVAO.Bind();
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	mQuadVAO.Unbind();

	glDisable(GL_BLEND);

	mLightingPipeline.Unbind();
	mLightingPass.End();

	mLights.clear();
}

void RenderingEngine::RunPostProcessingPass()
{
	if (mFilters.empty()) {
		mLastSceneTexture = mLightingBuffer->GetColorTexture(0);
		glClear(GL_COLOR_BUFFER_BIT);
		DrawQuad({ 0.0f, 0.0f }, { 1.0f, 1.0f }, mLastSceneTexture);
		return;
	}
	
	mPostProcessingPass.Begin();
	mQuadVAO.Bind();

	Texture* output = nullptr;
	for (auto&& filter : mFilters) {
		output = RunSingleFilter(filter, output);
	}

	mQuadVAO.Unbind();
	mPostProcessingPass.End();

	mLastSceneTexture = output;
	glClear(GL_COLOR_BUFFER_BIT);
	DrawQuad({ 0.0f, 0.0f }, { 1.0f, 1.0f }, output);
}

void RenderingEngine::RunSingleDirectionalShadowPass(const Light& light)
{
	if (light.type == LightType::point) return;
	if (!light.shadowsCaster) return;

	mShadowPass.Begin();
	mShadowPipeline.Bind();

	const Matrix identity = MatrixIdentity();

	mShadowShader->SetUniformMatrix4f("uLightSpaceMatrix", light.GetViewProjection());

	for (auto&& drawCall : mDrawCalls) {
		auto& material = mMaterials[drawCall.materialId];
		if (!material.castsShadows) continue;

		mShadowShader->SetUniformMatrix4f("uModelMatrix", drawCall.transform.value_or(identity));
		drawCall.mesh->Draw();
	}

	mShadowPipeline.Unbind();
	mShadowPass.End();
}

void RenderingEngine::RunSinglePointShadowPass(const Light& light)
{
	if (light.type != LightType::point) return;
	if (!light.shadowsCaster) return;

	mPointShadowPass.Begin();
	mPointShadowPipeline.Bind();

	const Matrix identity = MatrixIdentity();

	mPointShadowShader->SetUniform("uLightPosition", light.position);
	mPointShadowShader->SetUniform("uFar", light.radius);

	//mPointShadowShader->SetUniformBlockBinding("LightSpaceMatrices", 3);

	//std::vector<Matrix> matrices;
	for (u32 face = 0; face < 6; face++) {
		//matrices.push_back(light.GetViewProjection(face));
		mPointShadowShader->SetUniformMatrix4f(
			fmt::format("uLightSpaceMatrix[{}]", face),
			light.GetViewProjection(face)
		);
	}

	for (auto&& drawCall : mDrawCalls) {
		auto& material = mMaterials[drawCall.materialId];
		if (!material.castsShadows) continue;

		mPointShadowShader->SetUniformMatrix4f("uModelMatrix", drawCall.transform.value_or(identity));
		drawCall.mesh->Draw();
	}

	mPointShadowPipeline.Unbind();
	mPointShadowPass.End();
}

Texture* RenderingEngine::RunSingleFilter(Filter* filter, Texture* initialTexture)
{
	u32 drawBuffer = 0;
	bool firstTime = true;

	filter->Bind();
	filter->Reset();
	for (u32 pass = 0; pass < filter->GetPassCount(); pass++) {
		u32 src = drawBuffer;
		u32 dst = 1 - drawBuffer;

		Texture* prevPassTex = nullptr;
		if (firstTime) {
			if (!initialTexture) {
				switch (filter->GetInput()) {
					case FilterInput::lightingPass:
						prevPassTex = mLightingBuffer->GetColorTexture(0);
						break;
					case FilterInput::diffuseRoughness:
						prevPassTex = mGBuffer->GetColorTexture(0);
						break;
					case FilterInput::emissiveMetallic:
						prevPassTex = mGBuffer->GetColorTexture(1);
						break;
					case FilterInput::position:
						prevPassTex = mGBuffer->GetColorTexture(2);
						break;
					case FilterInput::normal:
						prevPassTex = mGBuffer->GetColorTexture(3);
						break;
				}
			}
			else {
				prevPassTex = initialTexture;
			}
			firstTime = false;
		}
		else {
			prevPassTex = mPingPongBuffer->GetColorTexture(src);
		}

		if (!prevPassTex) continue;

		prevPassTex->GenerateMipmaps();

		mPingPongBuffer->SetDrawBuffers({ dst });

		// Setup texture channels
		std::vector<GLuint64> channels;

		channels.push_back(prevPassTex->GetBindlessHandle()); // previous pass (0)
		channels.push_back(mLightingBuffer->GetColorTexture(0)->GetBindlessHandle()); // lighting pass (original) (1)
		channels.push_back(mGBuffer->GetColorTexture(0)->GetBindlessHandle()); // diffuse, roughness (2)
		channels.push_back(mGBuffer->GetColorTexture(1)->GetBindlessHandle()); // emissive, metallic (3)
		channels.push_back(mGBuffer->GetColorTexture(2)->GetBindlessHandle()); // position (4)
		channels.push_back(mGBuffer->GetColorTexture(3)->GetBindlessHandle()); // normal (5)

		filter->OnSetup();
		if (filter->HasUniform("uResolution")) {
			filter->SetUniform2f("uResolution", Vector2(mGBuffer->GetWidth(), mGBuffer->GetHeight()));
		}

		mTexturesSSBO.Bind();
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
		mTexturesSSBO.Update(channels.data(), channels.size());

		// Draw quad
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		drawBuffer = 1 - drawBuffer;
	}
	filter->Unbind();

	return mPingPongBuffer->GetColorTexture(drawBuffer);
}

void RenderingEngine::BuildSSBOMaterialData(
	std::vector<GLuint64>& textureData,
	std::vector<MaterialShaderData>& materialData
)
{
	u32 textureIndex = 0;
	for (const auto& mat : mMaterials) {
		MaterialShaderData data{
			.roughness = mat.roughness,
			.metallic = mat.metallic,
			.emissive = mat.emissive,
			.diffuseColor = {
				mat.diffuseColor.x,
				mat.diffuseColor.y,
				mat.diffuseColor.z,
				mat.diffuseColor.w
			},
			.diffuseTextureId = -1,
			.normalTextureId = -1,
			.roughnessTextureId = -1,
			.metallicTextureId = -1,
			.emissiveTextureId = -1,
			.displacementTextureId = -1,
		};

		// texture setup
		for (size_t i = 0; i < size_t(TextureSlot::count); i++) {
			auto& tex = mat.textures[i];
			if (tex.expired()) continue;

			const auto texture = tex.lock();
			textureData.push_back(texture->GetBindlessHandle());

			switch (static_cast<TextureSlot>(i)) {
				case TextureSlot::diffuse:
					data.diffuseTextureId = textureIndex;
					break;
				case TextureSlot::normal:
					data.normalTextureId = textureIndex;
					break;
				case TextureSlot::roughness:
					data.roughnessTextureId = textureIndex;
					break;
				case TextureSlot::metallic:
					data.metallicTextureId = textureIndex;
					break;
				case TextureSlot::emissive:
					data.emissiveTextureId = textureIndex;
					break;
				case TextureSlot::displacement:
					data.displacementTextureId = textureIndex;
					break;
			}

			textureIndex++;
		}

		materialData.push_back(data);
	}
}

void RenderingEngine::DrawQuad(
	const Vector2& position,
	const Vector2& scale,
	Texture* texture,
	bool isDepth,
	f32 zNear,
	f32 zFar
)
{
	mQuadVAO.Bind();
	mQuadShader->Bind();
	mQuadShader->SetUniform2f("uOffset", position);
	mQuadShader->SetUniform2f("uScale", scale);
	mQuadShader->SetUniform("uIsDepth", isDepth);
	mQuadShader->SetUniform("uNear", zNear);
	mQuadShader->SetUniform("uFar", zFar);

	if (texture) {
		texture->MakeResident();
		glUniformHandleui64ARB(mQuadShader->GetUniformLocation("uTexture"), texture->GetBindlessHandle());
	}
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	mQuadShader->Unbind();
	mQuadVAO.Unbind();
}
