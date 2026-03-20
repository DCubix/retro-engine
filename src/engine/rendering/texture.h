#pragma once

#include "opengl.hpp"
#include "../external/glad/glad.h"
#include "../external/raymath/raymath.h"

enum class TextureType {
	texture2D = GL_TEXTURE_2D,
	textureCubeMap = GL_TEXTURE_CUBE_MAP
};

enum class TextureFormat {
	R = 0,
	RG,
	RGB,
	RGBA,
	Rf,
	RGf,
	RGBf,
	RGBAf,
	depth,
	depthStencil
};

enum class TextureWrapMode {
	clampToEdge = GL_CLAMP_TO_EDGE,
	clampToBorder = GL_CLAMP_TO_BORDER,
	repeat = GL_REPEAT,
	mirroredRepeat = GL_MIRRORED_REPEAT
};

enum class TextureFilterMode {
	nearest = GL_NEAREST,
	linear = GL_LINEAR,
	nearestMipmapNearest = GL_NEAREST_MIPMAP_NEAREST,
	linearMipmapNearest = GL_LINEAR_MIPMAP_NEAREST,
	nearestMipmapLinear = GL_NEAREST_MIPMAP_LINEAR,
	linearMipmapLinear = GL_LINEAR_MIPMAP_LINEAR
};

struct SamplerDescription {
	TextureWrapMode wrapS{ TextureWrapMode::clampToEdge };
	TextureWrapMode wrapT{ TextureWrapMode::clampToEdge };
	TextureWrapMode wrapR{ TextureWrapMode::clampToEdge };
	TextureFilterMode minFilter{ TextureFilterMode::linear };
	TextureFilterMode magFilter{ TextureFilterMode::linear };
	float maxAnisotropy{ 1.0f };
};

struct TextureDescription {
	TextureType type{ TextureType::texture2D };

	u32 width;
	u32 height;
	u32 depth{ 0 };

	SamplerDescription samplerDescription{};

	/**
	* layer[0] = GL_TEXTURE_CUBE_MAP_POSITIVE_X / base
	* layer[1] = GL_TEXTURE_CUBE_MAP_NEGATIVE_X
	* layer[2] = GL_TEXTURE_CUBE_MAP_POSITIVE_Y
	* layer[3] = GL_TEXTURE_CUBE_MAP_NEGATIVE_Y
	* layer[4] = GL_TEXTURE_CUBE_MAP_POSITIVE_Z
	* layer[5] = GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
	*/
	byte* layers[6]{ 0 };

	TextureFormat format{ TextureFormat::RGBA };
};

//class Sampler : public IGPUResource {
//public:
//	Sampler() = default;
//	Sampler(const SamplerDescription& description);
//
//	GLuint GetID() const override { return mID; }
//
//	void Bind() {
//		glBindSampler(mUnit, mID);
//	}
//	void Unbind() {
//		glBindSampler(mUnit, 0);
//	}
//
//	Sampler& AtUnit(u32 unit) {
//		mUnit = unit;
//		return *this;
//	}
//
//	void Destroy() override;
//
//private:
//	GLuint mID;
//	u32 mUnit{ 0 };
//};

class Texture : public IGPUResource {
public:
	Texture() = default;
	Texture(const TextureDescription& description);

	void Resize(u32 width, u32 height);
	void Update(const byte* pixels);

	GLuint GetID() const override { return mID; }
	GLuint64 GetBindlessHandle() const { return mBindlessHandle; }
	TextureType GetType() const { return mType; }

	void Activate(u32 unit) const {
		glActiveTexture(GL_TEXTURE0 + unit);
	}

	//void BindSampler(Sampler& sampler, u32 unit = 0) {
	//	sampler.AtUnit(0).Bind();
	//	Activate(unit);
	//	Bind();
	//}

	//void UnbindSampler(Sampler& sampler) {
	//	Unbind();
	//	sampler.Unbind();
	//}

	void Bind() override {
		Activate(0);
		glBindTexture(static_cast<GLenum>(mType), mID);
	}

	void Unbind() override {
		glBindTexture(static_cast<GLenum>(mType), 0);
	}

	u32 GetWidth() const { return mWidth; }
	u32 GetHeight() const { return mHeight; }
	u32 GetDepth() const { return mDepth; }

	static Tup<GLenum, GLenum, GLenum> MapFormat(TextureFormat fmt) {
		switch (fmt) {
			case TextureFormat::R: return { GL_R8, GL_RED, GL_UNSIGNED_BYTE };
			case TextureFormat::RG: return { GL_RG8, GL_RG, GL_UNSIGNED_BYTE };
			case TextureFormat::RGB: return { GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE };
			case TextureFormat::RGBA: return { GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE };
			case TextureFormat::Rf: return { GL_R32F, GL_RED, GL_FLOAT };
			case TextureFormat::RGf: return { GL_RG32F, GL_RG, GL_FLOAT };
			case TextureFormat::RGBf: return { GL_RGB32F, GL_RGB, GL_FLOAT };
			case TextureFormat::RGBAf: return { GL_RGBA32F, GL_RGBA, GL_FLOAT };
			case TextureFormat::depth: return { GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT };
			case TextureFormat::depthStencil: return { GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8 };
			default: return { GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE };
		}
	}

	/// <summary>
	/// Make the texture non-resident in GPU memory.
	/// </summary>
	void Invalidate();
	void MakeResident();

	void GenerateMipmaps();
	void UpdateParameters(const TextureDescription& description);

	void Destroy() override;

protected:
	GLuint mID{ 0 };
	GLuint64 mBindlessHandle{ 0 };
	u32 mWidth{ 0 },
		mHeight{ 0 },
		mDepth{ 0 };
	TextureType mType;
	TextureFormat mFormat{ TextureFormat::RGBA };

	void InitTexture2D(const TextureDescription& description);
	void InitTextureCubeMap(const TextureDescription& description);
};
