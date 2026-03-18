#pragma once

#include "../utils/custom_types.h"
#include "../external/raymath/raymath.h"
#include "texture.h"
#include "material_shader.h"

#include <functional>

enum class TextureSlot : u8 {
	diffuse = 0,
	normal,
	roughness,
	metallic,
	emissive,
	displacement,
	count
};

struct RENGINE_API Material {
	WPtr<Texture> textures[(size_t)TextureSlot::count];
	WPtr<MaterialShader> customShader;

	float roughness{ 0.5f };
	float metallic{ 0.0f };
	float emissive{ 0.0f };

	Vector4 diffuseColor{ 1.0f, 1.0f, 1.0f, 1.0f };

	bool isTransparent{ false };
	bool castsShadows{ true };

	size_t GetID() const {
		size_t hash = 0;
		for (size_t i = 0; i < (size_t)TextureSlot::count; ++i) {
			if (!textures[i].expired()) {
				const auto texture = textures[i].lock();
				hash ^= texture->GetID();
			}
		}
		hash ^= std::hash<float>()(roughness);
		hash ^= std::hash<float>()(metallic);
		hash ^= std::hash<float>()(emissive);
		hash ^= std::hash<float>()(diffuseColor.x);
		hash ^= std::hash<float>()(diffuseColor.y);
		hash ^= std::hash<float>()(diffuseColor.z);
		hash ^= std::hash<float>()(diffuseColor.w);
		hash ^= std::hash<bool>()(castsShadows);
		hash ^= std::hash<bool>()(isTransparent);
		return hash;
	}

	void SetTexture(TextureSlot slot, const WPtr<Texture>& texture) {
		textures[(size_t)slot] = texture;
	}

	void SetTexture(TextureSlot slot, const SPtr<Texture>& texture) {
		textures[(size_t)slot] = texture;
	}
};

struct RENGINE_API alignas(16) MaterialShaderData {
	float roughness{ 0.5f };
	float metallic{ 0.0f };
	float emissive{ 0.0f };
	float _pad{ 0.0f };

	alignas(16) float diffuseColor[4]{ 1.0f, 1.0f, 1.0f, 1.0f };

	int diffuseTextureId;
	int normalTextureId;
	int roughnessTextureId;
	int metallicTextureId;
	int emissiveTextureId;
	int displacementTextureId;

	int _pad1{ 0 };
	int _pad2{ 0 };
};