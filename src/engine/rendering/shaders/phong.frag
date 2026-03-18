#version 460 core
#extension GL_ARB_bindless_texture : require

out vec4 FragColor;

#define MAX_TEXTURES 6 // max textures per material

#define TEX_DIFFUSE 0
#define TEX_NORMAL 1
#define TEX_ROUGHNESS 2
#define TEX_METALLIC 3
#define TEX_EMISSIVE 4
#define TEX_DISPLACEMENT 5

#define MIN_POM_LAYERS 8.0
#define MAX_POM_LAYERS 32.0

// Blinn-Phong lighting model based on roughness/metallic workflow
#define MIN_POWER 1.0
#define MAX_POWER 256.0
#define MAX_INTENSITY 0.9

in DATA {
	vec3 position;
	vec3 normal;
	vec3 tangent;
	vec2 texCoord;
	vec4 color;

	// tangent space 
	vec3 tangentPosition;
	vec3 tangentViewPosition;
	vec3 tangentLightPosition;
} vsin;

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

// All the textures
layout(std430, binding = 0) buffer AllTextures {
	sampler2D textures[];
};

// All the materials
layout(std430, binding = 1) buffer AllMaterials {
	Material materials[];
};

uniform uint uMaterialId;
uniform vec3 uViewPosition;

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

vec4 GetTextureColor(uint materialId, uint textureIndex, vec2 texCoord) {
	int id = GetTextureId(materialId, textureIndex);
	if (id >= 0) {
		return texture(textures[id], texCoord);
	}
	return vec4(1.0);
}

vec2 fxParallaxOcclusionMapping(vec2 uv, float heightScale, vec3 V) {
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
	float currentDepth = 1.0 - GetTextureColor(uMaterialId, TEX_DISPLACEMENT, currentUV).r;
  
	while (currentLayerDepth < currentDepth) {
		// shift texture coordinates along direction of P
		currentUV -= deltaUV;
		// get depthmap value at current texture coordinates
		currentDepth = 1.0 - GetTextureColor(uMaterialId, TEX_DISPLACEMENT, currentUV).r;
		// get depth of next layer
		currentLayerDepth += layerDepth;
	}

	// Apply Occlusion (interpolation with prev value)
	vec2 prevTexCoords = currentUV + deltaUV;
	float afterDepth  = currentDepth - currentLayerDepth;
	float beforeDepth =
		(1.0 - GetTextureColor(uMaterialId, TEX_DISPLACEMENT, prevTexCoords).r) - currentLayerDepth + layerDepth;
	float weight = afterDepth / (afterDepth - beforeDepth);
	currentUV = prevTexCoords * weight + currentUV * (1.0f - weight);

	// Get rid of anything outside the normal range
	if (currentUV.x > 1.0 || currentUV.y > 1.0 || currentUV.x < 0.0 || currentUV.y < 0.0)
		discard;

	return currentUV;
}

void main() {
	// Compute tangents and bitangents
	vec3 T = normalize(vsin.tangent);
	vec3 N = normalize(vsin.normal);
	vec3 L = normalize(vsin.tangentLightPosition - vsin.tangentPosition);
	vec3 V = normalize(vsin.tangentViewPosition - vsin.tangentPosition);
	vec3 H = normalize(L + V);

	vec2 uv = fxParallaxOcclusionMapping(vsin.texCoord, 0.05, V);

	Material mat = materials[uMaterialId];

	float roughness = mat.roughness;
	float metallic = mat.metallic;
	vec4 diffuseColor = vsin.color * mat.diffuseColor;
	vec3 emissionColor = vec3(0.0);

	if (GetTextureId(uMaterialId, TEX_DIFFUSE) >= 0) {
		diffuseColor *= GetTextureColor(uMaterialId, TEX_DIFFUSE, uv);
	}

	if (GetTextureId(uMaterialId, TEX_EMISSIVE) >= 0) {
		emissionColor = GetTextureColor(uMaterialId, TEX_EMISSIVE, uv).rgb * mat.emissive;
	} else {
		emissionColor = diffuseColor.rgb * mat.emissive;
	}

	if (GetTextureId(uMaterialId, TEX_NORMAL) >= 0) {
		vec4 normalMap = GetTextureColor(uMaterialId, TEX_NORMAL, uv);
		N = normalize(normalMap.rgb * 2.0 - 1.0);
	}

	if (GetTextureId(uMaterialId, TEX_ROUGHNESS) >= 0) {
		roughness *= GetTextureColor(uMaterialId, TEX_ROUGHNESS, uv).r;
	}

	if (GetTextureId(uMaterialId, TEX_METALLIC) >= 0) {
		metallic *= GetTextureColor(uMaterialId, TEX_METALLIC, uv).r;
	}

	vec3 finalDiffuseColor = (1.0 - metallic) * diffuseColor.rgb;
	vec3 finalSpecularColor = mix(vec3(1.0), diffuseColor.rgb, metallic);

	// Simple lighting
	// Diffuse 
	float NdotL = clamp(dot(N, L), 0.0, 1.0);
	vec3 diffuse = mat.diffuseColor.rgb * NdotL;

	// Specular Blinn-Phong
	roughness = clamp(roughness, 0.04, 1.0);
	float specularPower = MIN_POWER + pow(1.0 - roughness, 2.0) * (MAX_POWER - MIN_POWER);
	float specularIntensity = (1.0 - roughness) * MAX_INTENSITY;

	float NdotH = max(dot(N, H), 0.0);
	float specularFactor = clamp(pow(NdotH, specularPower) * specularIntensity, 0.0, 1.0);

	// Rim lighting
	float NdotV = max(dot(N, V), 0.0);
	float rimFactor = clamp(pow(1.0 - NdotV, 4.0), 0.0, 1.0);
	vec3 rimColor = mix(vec3(0.0), diffuseColor.rgb, rimFactor) * NdotL;

	// Ambient
	vec3 ambient = vec3(0.07);

	vec3 finalColor = (ambient + (diffuse * finalDiffuseColor) + (finalSpecularColor * specularFactor) + rimColor) + emissionColor;

	FragColor = vec4(finalColor, 1.0);//vec4((ambient + diffuse + specular) * diffuseColor.rgb, diffuseColor.a);
}