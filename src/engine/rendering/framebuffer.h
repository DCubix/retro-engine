#pragma once

#include "texture.h"
#include <array>
#include <vector>

enum class FrameBufferBindMode {
	read = GL_READ_FRAMEBUFFER,
	write = GL_DRAW_FRAMEBUFFER,
	readWrite = GL_FRAMEBUFFER
};

enum class RenderBufferFormat {
	depth = GL_DEPTH_COMPONENT24,
	depthStencil = GL_DEPTH24_STENCIL8,
};

struct FrameBufferDescription {
	u32 width{ 0 };
	u32 height{ 0 };
	
	std::vector<TextureFormat> colorAttachments;
	bool colorIsCubeMap{ false };

	struct {
		bool enabled{ false };
		bool withStencil{ false };
		bool isCubeMap{ false };
	} depth;

	struct {
		bool enabled{ false };
		RenderBufferFormat format{ RenderBufferFormat::depthStencil };
	} renderBuffer;
};

class FrameBuffer : public IGPUResource {
public:
	FrameBuffer() = default;
	FrameBuffer(const FrameBufferDescription& description);

	void BindAs(FrameBufferBindMode mode);
	void Bind() { BindAs(FrameBufferBindMode::readWrite); }
	void Unbind();

	void Resize(u32 width, u32 height);

	Texture* GetColorTexture(u32 index) {
		if (index >= mColorTextures.size()) {
			return nullptr;
		}
		return mColorTextures[index].get();
	}

	Texture* GetDepthTexture() {
		return mDepthTexture.get();
	}

	void SetReadBuffer(u32 index);
	void SetDrawBuffers(const std::vector<u32>& indices);

	void Destroy() override;

	GLuint GetID() const { return mID; }
	u32 GetWidth() const { return mWidth; }
	u32 GetHeight() const { return mHeight; }

private:
	std::array<UPtr<Texture>, 8> mColorTextures;
	UPtr<Texture> mDepthTexture{ nullptr };

	GLuint mID{ 0 };
	GLuint mRBO{ 0 };

	u32 mWidth{ 0 };
	u32 mHeight{ 0 };

	FrameBufferDescription mSavedDescription;

	int mSavedViewport[4]{ 0, 0, 0, 0 };
	FrameBufferBindMode mSavedBindMode;
protected:
	void Create(const FrameBufferDescription& description);
};