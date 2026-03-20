#pragma once

#include "material.h"
#include "mesh.h"
#include "pipeline.h"
#include "pass.h"
#include "shader.h"
#include "buffer.h"
#include "light.h"
#include "filters.h"
#include "debug_draw.h"

#include "../utils/profiler.h"

#include <ranges>
#include <map>
#include <array>

using MaterialID = size_t;

struct DrawCall {
	Mesh* mesh;
	PrimitiveType primitiveType{ PrimitiveType::triangles };
	MaterialID materialId;
	u32 instanceCount{ 0 };
	Opt<Matrix> transform;
};

class RenderingEngine {
public:
	RenderingEngine() = default;
	RenderingEngine(u32 framebufferWidth, u32 framebufferHeight);

	void AddToRender(Mesh* mesh, const Material& material, const Opt<Matrix>& transform);
	void AddToRender(const Light& light);

	void AddFilter(Filter* filter);
	void RemoveFilter(Filter* filter);

	Vector3 GetAmbientColor() const { return mAmbientColor; }
	void SetAmbientColor(const Vector3& color) { mAmbientColor = color; }

	Vector3 GetViewPosition() const;
	void SetViewPosition(const Vector3& position) { mViewPosition = position; }

	Matrix GetViewProjectionMatrix() const { return mViewMatrix * mProjectionMatrix; }

	void CameraSetup(const Matrix& projectionMatrix, const Matrix& viewMatrix);

	u32 GetFramebufferWidth() const { return mGBuffer->GetWidth(); }
	u32 GetFramebufferHeight() const { return mGBuffer->GetHeight(); }

	DebugDraw* GetDebugDraw() { return mDebugDraw.get(); }
	const Profiler& GetProfiler() const { return mProfiler; }

	void Render();
private:
	Matrix mProjectionMatrix{ 1.0f };
	Matrix mViewMatrix{ 1.0f };
	Vector3 mViewPosition{ 0.0f, 0.0f, 0.0f };

	Vector3 mAmbientColor{ 0.02f, 0.02f, 0.02f };

	std::vector<Filter*> mFilters;

	std::vector<Light> mLights;
	std::vector<Material> mMaterials;
	std::vector<DrawCall> mDrawCalls;

	Pipeline mGBufferPipeline;
	Pass mGBufferPass;
	UPtr<Shader> mGBufferShader;
	UPtr<FrameBuffer> mGBuffer;

	Pipeline mLightingPipeline;
	Pass mLightingPass;
	UPtr<Shader> mLightingShader;
	UPtr<FrameBuffer> mLightingBuffer;

	Pass mPostProcessingPass;
	UPtr<FrameBuffer> mPingPongBuffer;

	Pipeline mShadowPipeline;
	Pass mShadowPass;
	UPtr<FrameBuffer> mShadowBuffer;
	UPtr<Shader> mShadowShader;

	Pipeline mPointShadowPipeline;
	Pass mPointShadowPass;
	UPtr<FrameBuffer> mPointShadowBuffer;
	UPtr<Shader> mPointShadowShader;

	UPtr<Shader> mQuadShader;
	VertexArray mQuadVAO;

	Buffer<MaterialShaderData, BufferType::shaderStorageBuffer> mMaterialsSSBO;
	Buffer<GLuint64, BufferType::shaderStorageBuffer> mTexturesSSBO;
	Buffer<LightShaderData, BufferType::uniformBuffer> mLightUBO;
	Buffer<Matrix, BufferType::uniformBuffer> mLightMatricesUBO;

	UPtr<DebugDraw> mDebugDraw;

	Profiler mProfiler{};

	void BuildSSBOMaterialData(
		std::vector<GLuint64>& textureData,
		std::vector<MaterialShaderData>& materialData
	);

	void DrawQuad(
		const Vector2& position,
		const Vector2& scale,
		Texture* texture,
		bool isDepth = false,
		f32 zNear = 0.0f,
		f32 zFar = 1.0f
	);

	void RunGBufferPass();
	void RunLightingPass();
	void RunPostProcessingPass();
	void RunSingleDirectionalShadowPass(const Light& light);
	void RunSinglePointShadowPass(const Light& light);

	Texture* RunSingleFilter(Filter* filter, Texture* initialTexture);
};