#include "texture.h"

#include <stdexcept>

#include "../utils/logger.h"

Texture::Texture(const TextureDescription& description)
{
    mWidth = description.width;
    mHeight = description.height;
    mDepth = description.depth;
    mType = description.type;
    mFormat = description.format;

    glCreateTextures(static_cast<GLenum>(mType), 1, &mID);

    switch (description.type) {
        case TextureType::texture2D: InitTexture2D(description); break;
        case TextureType::textureCubeMap: InitTextureCubeMap(description); break;
    }

	if (description.samplerDescription.minFilter == TextureFilterMode::linearMipmapLinear ||
		description.samplerDescription.minFilter == TextureFilterMode::linearMipmapNearest ||
		description.samplerDescription.minFilter == TextureFilterMode::nearestMipmapLinear ||
		description.samplerDescription.minFilter == TextureFilterMode::nearestMipmapNearest) {
	    GenerateMipmaps();
	}

    // Make the texture bindless
    MakeResident();
}

void Texture::Resize(u32 width, u32 height) {
    if (mType != TextureType::texture2D) {
        utils::Fail("Cannot resize non-2D texture");
    }
    mWidth = width;
    mHeight = height;

    const auto [internalFormat, format, type] = MapFormat(mFormat);

    glTextureStorage2D(
        mID,
        1,
        internalFormat,
        mWidth, mHeight
    );
    glTextureSubImage2D(
        mID,
        0,
        0, 0,
        mWidth, mHeight,
        format,
        type,
        nullptr
    );

	GenerateMipmaps();

    // Update the bindless handle
    glMakeTextureHandleNonResidentARB(mBindlessHandle);
    mBindlessHandle = glGetTextureHandleARB(mID);
    glMakeTextureHandleResidentARB(mBindlessHandle);
}

void Texture::Update(const byte* pixels)
{
	if (mType != TextureType::texture2D) {
		utils::LogE("Cannot update non-2D textures");
	}

	const auto [_, format, type] = MapFormat(mFormat);
	glTextureSubImage2D(
		mID,
		0,
		0, 0,
		mWidth, mHeight,
		format,
		type,
		pixels
	);
}

void Texture::Invalidate()
{
	if (mBindlessHandle) {
		glMakeTextureHandleNonResidentARB(mBindlessHandle);
		mBindlessHandle = 0;
	}
}

void Texture::MakeResident()
{
	if (!mBindlessHandle) {
        mBindlessHandle = glGetTextureHandleARB(mID);
	}
	if (glIsTextureHandleResidentARB(mBindlessHandle)) {
		return;
	}
	glMakeTextureHandleResidentARB(mBindlessHandle);
}

void Texture::GenerateMipmaps() {
    glGenerateTextureMipmap(mID);
}

void Texture::UpdateParameters(const TextureDescription& description)
{
    glTextureParameteri(mID, GL_TEXTURE_WRAP_S, static_cast<GLenum>(description.samplerDescription.wrapS));
    glTextureParameteri(mID, GL_TEXTURE_WRAP_T, static_cast<GLenum>(description.samplerDescription.wrapT));
    if (mType == TextureType::textureCubeMap) {
        glTextureParameteri(mID, GL_TEXTURE_WRAP_R, static_cast<GLenum>(description.samplerDescription.wrapR));
    }
    glTextureParameteri(mID, GL_TEXTURE_MIN_FILTER, static_cast<GLenum>(description.samplerDescription.minFilter));
    glTextureParameteri(mID, GL_TEXTURE_MAG_FILTER, static_cast<GLenum>(description.samplerDescription.magFilter));
    glTextureParameterf(mID, GL_TEXTURE_MAX_ANISOTROPY, description.samplerDescription.maxAnisotropy);
    glTextureParameteri(mID, GL_TEXTURE_BASE_LEVEL, 0);

    f32 borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glTextureParameterfv(mID, GL_TEXTURE_BORDER_COLOR, borderColor);
}

void Texture::Destroy() {
    Invalidate();
    if (mID) {
        glDeleteTextures(1, &mID);
        mID = 0;
    }
}

void Texture::InitTexture2D(const TextureDescription& description) {
    const auto [internalFormat, format, type] = MapFormat(description.format);
    glTextureStorage2D(
        mID,
        1,
        internalFormat,
        description.width, description.height
    );
	UpdateParameters(description);
    glTextureSubImage2D(
        mID,
        0,
        0, 0,
        description.width, description.height,
        format,
        type,
        description.layers[0]
    );
}

void Texture::InitTextureCubeMap(const TextureDescription& description) {
    const auto [internalFormat, format, type] = MapFormat(description.format);
    const GLenum faces[6] = {
        GL_TEXTURE_CUBE_MAP_POSITIVE_X,
        GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
        GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
        GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
        GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
        GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
    };

	glTextureStorage2D(
		mID,
		1,
		internalFormat,
		description.width, description.height
	);
    UpdateParameters(description);

    for (int i = 0; i < 6; i++) {
		glTextureSubImage3D(
			mID,
			0,
			0, 0, i,
			description.width, description.height, 1,
			format,
			type,
			description.layers[i]
		);
    }
}

//Sampler::Sampler(const SamplerDescription& description)
//{
//	glCreateSamplers(1, &mID);
//	glSamplerParameteri(mID, GL_TEXTURE_WRAP_S, static_cast<GLenum>(description.wrapS));
//	glSamplerParameteri(mID, GL_TEXTURE_WRAP_T, static_cast<GLenum>(description.wrapT));
//	glSamplerParameteri(mID, GL_TEXTURE_WRAP_R, static_cast<GLenum>(description.wrapR));
//	glSamplerParameteri(mID, GL_TEXTURE_MIN_FILTER, static_cast<GLenum>(description.minFilter));
//	glSamplerParameteri(mID, GL_TEXTURE_MAG_FILTER, static_cast<GLenum>(description.magFilter));
//	glSamplerParameterf(mID, GL_TEXTURE_MAX_ANISOTROPY, description.maxAnisotropy);
//}
//
//void Sampler::Destroy()
//{
//	if (mID) {
//		glDeleteSamplers(1, &mID);
//		mID = 0;
//	}
//}
