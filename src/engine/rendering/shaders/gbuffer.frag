#version 460 core
#extension GL_ARB_bindless_texture : require

layout (location = 0) out vec4 gbDiffuseRoughness;
layout (location = 1) out vec4 gbEmissiveMetallic;
layout (location = 2) out vec3 gbPosition;
layout (location = 3) out vec3 gbNormal;

#define MAX_TEXTURES 6 // max textures per material

#define TEX_DIFFUSE 0
#define TEX_NORMAL 1
#define TEX_ROUGHNESS 2
#define TEX_METALLIC 3
#define TEX_EMISSIVE 4
#define TEX_DISPLACEMENT 5

#define MIN_POM_LAYERS 8.0
#define MAX_POM_LAYERS 32.0

struct Material {
	float roughness;
	float metallic;
	float emissive;
	float _padding0;

	vec4 diffuseColor;

	int diffuseTextureId;
	int normalTextureId;
	int roughnessTextureId;
	int metallicTextureId;
	int emissiveTextureId;
	int displacementTextureId;

	int _padding1;
	int _padding2;
};

struct ShaderOutput {
	vec4 diffuse;
	vec3 normal;
	vec3 emissive;
	float roughness;
	float metallic;
};

struct InputData {
	vec3 position;
	vec3 viewDirection;
	vec3 normal;
	vec3 tangent;
	vec2 texCoord;
	vec4 vertexColor;
	uint materialId;
};

in DATA {
	vec3 position;
	vec3 tangentPosition;
	vec3 tangentViewPosition;
	vec3 viewPosition;
	vec3 normal;
	vec3 tangent;
	vec2 texCoord;
	vec4 vertexColor;
	mat3 tbn;
} vsin;

// All the textures
layout(std430, binding = 0) buffer AllTextures {
	sampler2D textures[];
};

// All the materials
layout(std430, binding = 1) buffer AllMaterials {
	Material materials[];
};

uniform uint uMaterialId;

int GetTextureId(uint materialId, uint textureIndex) {
	Material mat = materials[materialId];
	switch (textureIndex) {
		case TEX_DIFFUSE: return mat.diffuseTextureId;
		case TEX_NORMAL: return mat.normalTextureId;
		case TEX_ROUGHNESS: return mat.roughnessTextureId;
		case TEX_METALLIC: return mat.metallicTextureId;
		case TEX_EMISSIVE: return mat.emissiveTextureId;
		case TEX_DISPLACEMENT: return mat.displacementTextureId;
	}
	return -1;
}

Material GetMaterial(uint materialId) {
	return materials[materialId];
}

vec4 SampleTexture(uint materialId, uint textureIndex, vec2 texCoord) {
	int id = GetTextureId(materialId, textureIndex);
	if (id >= 0) {
		return texture(textures[id], texCoord);
	}
	return vec4(1.0);
}

vec3 GetTriplanarBlend(vec3 normal) {
	vec3 blend = abs(normal);
	blend = normalize(max(blend, 0.00001));
	float b = blend.x + blend.y + blend.z;
	blend /= b;
	return blend;
}

vec4 SampleTriplanar(
	uint materialId,
	uint textureIndex,
	vec3 worldPos,
	vec3 normal,
	float repeat
) {
	vec3 blending = GetTriplanarBlend(normal);
	vec4 xaxis = SampleTexture(materialId, textureIndex, worldPos.yz * repeat);
	vec4 yaxis = SampleTexture(materialId, textureIndex, worldPos.xz * repeat);
	vec4 zaxis = SampleTexture(materialId, textureIndex, worldPos.xy * repeat);
	return xaxis * blending.x + yaxis * blending.y + zaxis * blending.z;
}

vec4 SampleTriplanar(
	uint materialId,
	uint textureIndex,
	vec3 worldPos,
	vec3 normal
) {
	return SampleTriplanar(materialId, textureIndex, worldPos, normal, 1.0);
}

vec4 SampleTriplanar(
	sampler2D tex,
	vec3 worldPos,
	vec3 normal,
	float repeat
) {
	vec3 blending = GetTriplanarBlend(normal);
	vec4 xaxis = texture(tex, worldPos.yz * repeat);
	vec4 yaxis = texture(tex, worldPos.xz * repeat);
	vec4 zaxis = texture(tex, worldPos.xy * repeat);
	return xaxis * blending.x + yaxis * blending.y + zaxis * blending.z;
}

vec4 SampleTriplanar(
	sampler2D tex,
	vec3 worldPos,
	vec3 normal
) {
	return SampleTriplanar(tex, worldPos, normal, 1.0);
}

vec3 SampleNormalMap(uint materialId, vec2 texCoord) {
	vec4 normalMap = SampleTexture(materialId, TEX_NORMAL, texCoord);
	return normalize(vsin.tbn * (normalMap.rgb * 2.0 - 1.0));
}

vec2 ParallaxOcclusionMapping(vec2 uv, float heightScale, vec3 V) {
	if (GetTextureId(uMaterialId, TEX_DISPLACEMENT) < 0) {
		return uv;
	}

	float angle = max(dot(vec3(0.0, 0.0, 1.0), V), 0.0);

	float numLayers = mix(MAX_POM_LAYERS, MIN_POM_LAYERS, angle);
	float layerDepth = 1.0 / numLayers;

	float currentLayerDepth = 0.0;

	vec2 P = V.xy / V.z * heightScale;
	vec2 deltaUV = P / numLayers;

	vec2 currentUV = uv;
	float currentDepth = 1.0 - SampleTexture(uMaterialId, TEX_DISPLACEMENT, currentUV).r;
  
	while (currentLayerDepth < currentDepth) {
		// shift texture coordinates along direction of P
		currentUV -= deltaUV;
		// get depthmap value at current texture coordinates
		currentDepth = 1.0 - SampleTexture(uMaterialId, TEX_DISPLACEMENT, currentUV).r;
		// get depth of next layer
		currentLayerDepth += layerDepth;
	}

	// Apply Occlusion (interpolation with prev value)
	vec2 prevTexCoords = currentUV + deltaUV;
	float afterDepth  = currentDepth - currentLayerDepth;
	float beforeDepth =
		(1.0 - SampleTexture(uMaterialId, TEX_DISPLACEMENT, prevTexCoords).r) - currentLayerDepth + layerDepth;
	float weight = afterDepth / (afterDepth - beforeDepth);
	currentUV = prevTexCoords * weight + currentUV * (1.0f - weight);

	// Get rid of anything outside the normal range
	if (currentUV.x > 1.0 || currentUV.y > 1.0 || currentUV.x < 0.0 || currentUV.y < 0.0)
		discard;

	return currentUV;
}

ShaderOutput DefaultFragmentShader(in InputData i) {
	Material mat = GetMaterial(i.materialId);
	ShaderOutput o;
	o.diffuse =
		SampleTexture(i.materialId, TEX_DIFFUSE, i.texCoord) *
		i.vertexColor *
		mat.diffuseColor;
	o.normal =
		GetTextureId(i.materialId, TEX_NORMAL) >= 0
			? SampleNormalMap(i.materialId, i.texCoord)
			: i.normal;
	o.emissive =
		SampleTexture(i.materialId, TEX_EMISSIVE, i.texCoord).rgb *
		mat.emissive;
	o.roughness =
		max(SampleTexture(i.materialId, TEX_ROUGHNESS, i.texCoord).r * mat.roughness, 0.04);
	o.metallic =
		SampleTexture(i.materialId, TEX_METALLIC, i.texCoord).r *
		mat.metallic;
	return o;
}

#ifndef CUSTOM_SHADER_FS
ShaderOutput FragmentShader(in InputData i) {
	return DefaultFragmentShader(i);
}
#else
#include "custom_fragment_shader"
#endif

void main() {
	// Compute tangents and bitangents
	vec3 T = normalize(vsin.tangent);
	vec3 Nws = normalize(vsin.normal);
	vec3 V = normalize(vsin.tangentViewPosition - vsin.tangentPosition);
	vec2 uv = ParallaxOcclusionMapping(vsin.texCoord, 0.05, V);

	InputData i;
	i.position = vsin.position;
	i.viewDirection = V;
	i.normal = Nws;
	i.tangent = T;
	i.texCoord = uv;
	i.vertexColor = vsin.vertexColor;
	i.materialId = uMaterialId;

	ShaderOutput o = FragmentShader(i);

	gbDiffuseRoughness = vec4(o.diffuse.rgb, o.roughness);
	gbEmissiveMetallic = vec4(o.emissive, o.metallic);
	gbPosition = vsin.position;
	gbNormal = o.normal;
}