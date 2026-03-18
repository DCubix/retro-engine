#include "framebuffer.h"

#include <cassert>
#include <vector>

#include "../utils/logger.h"

FrameBuffer::FrameBuffer(const FrameBufferDescription& description)
{
	Create(description);
}

void FrameBuffer::BindAs(FrameBufferBindMode mode)
{
	// save the current viewport
	glGetIntegerv(GL_VIEWPORT, mSavedViewport);
	glBindFramebuffer(static_cast<GLenum>(mode), mID);
	glViewport(0, 0, mWidth, mHeight);
	mSavedBindMode = mode;
}

void FrameBuffer::Unbind()
{
	glBindFramebuffer(static_cast<GLenum>(mSavedBindMode), 0);
	glViewport(mSavedViewport[0], mSavedViewport[1], mSavedViewport[2], mSavedViewport[3]);
}

void FrameBuffer::Resize(u32 width, u32 height)
{
	if (width == mWidth && height == mHeight) return;
	
	FrameBufferDescription description = mSavedDescription;
	description.width = width;
	description.height = height;

	Destroy();
	Create(description);
}

void FrameBuffer::Create(const FrameBufferDescription& description)
{
	assert(description.colorAttachments.size() <= 8 && "Max 8 color attachments supported");

	mWidth = description.width;
	mHeight = description.height;
	mSavedDescription = description;
	
	auto colorAttachments = description.colorAttachments;
	auto depthFormat = description.depth.enabled
		? (description.depth.withStencil ? TextureFormat::depthStencil : TextureFormat::depth)
		: TextureFormat::depth;
	auto rboFormat = description.renderBuffer.format;

	glCreateFramebuffers(1, &mID);

	std::vector<GLenum> drawBuffers;
	for (u32 i = 0; i < colorAttachments.size(); i++) {
		TextureDescription textureDesc{};
		textureDesc.width = mWidth;
		textureDesc.height = mHeight;
		textureDesc.format = colorAttachments[i];
		textureDesc.type = description.colorIsCubeMap ? TextureType::textureCubeMap : TextureType::texture2D;
		textureDesc.samplerDescription.wrapS = TextureWrapMode::clampToEdge;
		textureDesc.samplerDescription.wrapT = TextureWrapMode::clampToEdge;
		textureDesc.samplerDescription.wrapR = TextureWrapMode::clampToEdge;
		textureDesc.samplerDescription.minFilter = TextureFilterMode::linearMipmapLinear;
		textureDesc.samplerDescription.magFilter = TextureFilterMode::linear;

		auto texture = std::make_unique<Texture>(textureDesc);

		drawBuffers.push_back(GL_COLOR_ATTACHMENT0 + i);

		if (description.colorIsCubeMap) {
			for (u32 face = 0; face < 6; face++) {
				glNamedFramebufferTextureLayer(
					mID,
					drawBuffers.back(),
					texture->GetID(),
					0,
					face
				);
			}
		}
		else {
			glNamedFramebufferTexture(mID, drawBuffers.back(), texture->GetID(), 0);
		}

		mColorTextures[i] = std::move(texture);
	}

	glNamedFramebufferDrawBuffers(mID, static_cast<GLsizei>(drawBuffers.size()), drawBuffers.data());

	if (description.depth.enabled) {
		TextureDescription depthTextureDesc{};
		depthTextureDesc.width = mWidth;
		depthTextureDesc.height = mHeight;
		depthTextureDesc.format = description.depth.withStencil ? TextureFormat::depthStencil : TextureFormat::depth;
		depthTextureDesc.type = description.depth.isCubeMap ? TextureType::textureCubeMap : TextureType::texture2D;
		depthTextureDesc.samplerDescription.wrapS = TextureWrapMode::clampToBorder;
		depthTextureDesc.samplerDescription.wrapT = TextureWrapMode::clampToBorder;
		depthTextureDesc.samplerDescription.wrapR = TextureWrapMode::clampToBorder;
		depthTextureDesc.samplerDescription.minFilter = TextureFilterMode::linear;
		depthTextureDesc.samplerDescription.magFilter = TextureFilterMode::linear;

		mDepthTexture = std::make_unique<Texture>(depthTextureDesc);

		GLenum attachment = description.depth.withStencil ? GL_DEPTH_STENCIL_ATTACHMENT : GL_DEPTH_ATTACHMENT;
		
		if (description.depth.isCubeMap) {
			glNamedFramebufferTexture(mID, attachment, mDepthTexture->GetID(), 0);
			glNamedFramebufferDrawBuffer(mID, GL_NONE);
			glNamedFramebufferReadBuffer(mID, GL_NONE);
		}
		else {
			glNamedFramebufferTexture(mID, attachment, mDepthTexture->GetID(), 0);
		}

	}

	if (description.renderBuffer.enabled) {
		const GLenum format = description.renderBuffer.format == RenderBufferFormat::depth
			? GL_DEPTH_COMPONENT24
			: GL_DEPTH24_STENCIL8;
		const GLenum attachment = description.renderBuffer.format == RenderBufferFormat::depth
			? GL_DEPTH_ATTACHMENT
			: GL_DEPTH_STENCIL_ATTACHMENT;

		glCreateRenderbuffers(1, &mRBO);
		glNamedRenderbufferStorage(mRBO, format, mWidth, mHeight);
		glNamedFramebufferRenderbuffer(mID, attachment, GL_RENDERBUFFER, mRBO);
	}

	GLenum status = glCheckNamedFramebufferStatus(mID, GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		utils::LogE("Framebuffer not complete: {}", status);
	}
}

void FrameBuffer::SetReadBuffer(u32 index)
{
	if (index >= mColorTextures.size()) {
		glNamedFramebufferReadBuffer(mID, GL_NONE);
		return;
	}
	glNamedFramebufferReadBuffer(mID, GL_COLOR_ATTACHMENT0 + index);
}

void FrameBuffer::SetDrawBuffers(const std::vector<u32>& indices) {
	if (indices.empty()) {
		// set all
		std::vector<u32> drawBuffers;
		for (u32 i = 0; i < mSavedDescription.colorAttachments.size(); i++) {
			drawBuffers.push_back(i);
		}
		SetDrawBuffers(drawBuffers);
		return;
	}

	std::vector<GLenum> drawBuffers;
	for (const auto& index : indices) {
		drawBuffers.push_back(GL_COLOR_ATTACHMENT0 + index);
	}
	glNamedFramebufferDrawBuffers(
		mID,
		static_cast<GLsizei>(drawBuffers.size()),
		drawBuffers.data()
	);
}

void FrameBuffer::Destroy()
{
	if (mID) {
		glDeleteFramebuffers(1, &mID);
		mID = 0;
	}
	if (mRBO) {
		glDeleteRenderbuffers(1, &mRBO);
		mRBO = 0;
	}
	for (auto& texture : mColorTextures) {
		if (texture) texture.reset();
	}
	if (mDepthTexture) mDepthTexture.reset();
}