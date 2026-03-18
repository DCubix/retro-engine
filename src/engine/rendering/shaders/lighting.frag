#version 430 core
#extension GL_ARB_bindless_texture : require

#define PI 3.14159265

// Blinn-Phong lighting model based on roughness/metallic workflow
#define MIN_POWER 1.0
#define MAX_POWER 256.0
#define MAX_INTENSITY 0.9

out vec4 FragColor;
in vec2 vTexCoord;

layout (location = 0, bindless_sampler) uniform sampler2D ugbDiffuseRoughness;
layout (location = 1, bindless_sampler) uniform sampler2D ubgEmissiveMetallic;
layout (location = 2, bindless_sampler) uniform sampler2D ugbPosition;
layout (location = 3, bindless_sampler) uniform sampler2D ugbNormal;

struct Light {
	vec4 colorIntensity;
	vec4 positionRadius;
	vec4 directionCutoffAngle;
	int lightType;
	float _padding0;
	float _padding1;
	float _padding2;
};

layout(std430, binding = 2) buffer AllLights {
	Light lights[];
};
uniform uint uNumLights;

uniform vec3 uViewPosition;
uniform vec3 uAmbientColor;

void ComputeDirectionalLight(Light light, out vec3 L, out float attenuation) {
	L = -light.directionCutoffAngle.xyz;
	attenuation = 1.0;
}

void ComputePointLight(Light light, vec3 P, out vec3 L, out float attenuation) {
	L = light.positionRadius.xyz - P;
	float dist = length(L);
	L = normalize(L);

	if (dist > light.positionRadius.w) {
		attenuation = 0.0;
		return;
	}

	float r = light.positionRadius.w;
	float attCoef = 1.0 - clamp(dist / r, 0.0, 1.0);
	attenuation = attCoef * attCoef;
}

void ComputeSpotLight(Light light, vec3 P, out vec3 L, out float attenuation) {
	float atten = 0.0;
	ComputePointLight(light, P, L, atten);

	vec3 Ldir = light.directionCutoffAngle.xyz;
	float cutoffAngle = light.directionCutoffAngle.w * 0.5;
	float S = dot(L, normalize(-Ldir));
	float c = cos(cutoffAngle);
	if (S > c) {
		atten *= (1.0 - (1.0 - S) * 1.0 / (1.0 - c));
	} else {
		atten = 0.0;
	}

	attenuation = atten;
}

float OrenNayarDiffuse(
  vec3 lightDirection,
  vec3 viewDirection,
  vec3 surfaceNormal,
  float roughness,
  float albedo) {
  
  float LdotV = dot(lightDirection, viewDirection);
  float NdotL = dot(lightDirection, surfaceNormal);
  float NdotV = dot(surfaceNormal, viewDirection);

  float s = LdotV - NdotL * NdotV;
  float t = mix(1.0, max(NdotL, NdotV), step(0.0, s));

  float sigma2 = roughness * roughness;
  float A = 1.0 + sigma2 * (albedo / (sigma2 + 0.13) + 0.5 / (sigma2 + 0.33));
  float B = 0.45 * sigma2 / (sigma2 + 0.09);

  return albedo * max(0.0, NdotL) * (A + B * s / t) / PI;
}

vec3 CalculateLighting(Light light, vec3 P, vec3 N, vec3 V, vec4 diffRough, vec4 emissMetl) {
	vec3 L;
	float attenuation = 1.0;

	if (light.lightType == 0) {
		ComputeDirectionalLight(light, L, attenuation);
	} else if (light.lightType == 1) {
		ComputePointLight(light, P, L, attenuation);
	} else if (light.lightType == 2) {
		ComputeSpotLight(light, P, L, attenuation);
	}

	vec3 lightColor = light.colorIntensity.rgb;
	float lightIntensity = light.colorIntensity.a;
	
	float roughness = diffRough.a;
	float metallic = emissMetl.a;

	// Oren-Nayar diffuse model
	float lightFactor = OrenNayarDiffuse(L, V, N, roughness, 0.8) * attenuation;
	//

	vec3 diffuse = lightColor * lightFactor * lightIntensity;
	vec3 diffuseColor = diffRough.rgb;

	// Blinn-Phong specular
	vec3 H = normalize(L + V);
	float NoH = max(dot(N, H), 0.0);

	float specularPower = MIN_POWER + pow(1.0 - roughness, 2.0) * (MAX_POWER - MIN_POWER);
	float specularIntensity = (1.0 - roughness) * MAX_INTENSITY;

	float specularFactor = clamp(pow(NoH, specularPower) * specularIntensity, 0.0, 1.0);
	vec3 specularColor = mix(vec3(1.0), diffRough.rgb, metallic);
	vec3 specular = lightColor * specularFactor * lightFactor * lightIntensity;

	// rim lighting oriented at light vector
	float NoL = max(dot(N, L), 0.0);
	float rimFactor = clamp(pow(1.0 - NoL, 3.0), 0.0, 1.0);
	vec3 rimColor = lightColor * rimFactor * lightIntensity * lightFactor;

	vec3 finalColor = (
		(diffuse * diffuseColor) +
		(specular * specularColor) +
		rimColor * 0.8
	);

	return finalColor;
}

void main() {
	vec3 P = texture(ugbPosition, vTexCoord).xyz;
	vec3 N = texture(ugbNormal, vTexCoord).xyz;
	vec3 V = normalize(uViewPosition - P);
	vec4 diffRough = texture(ugbDiffuseRoughness, vTexCoord);
	vec4 emissMetl = texture(ubgEmissiveMetallic, vTexCoord);

	vec3 finalColor = uAmbientColor * diffRough.rgb;
	for (uint i = 0; i < uNumLights; i++) {
		Light light = lights[i];
		finalColor += CalculateLighting(light, P, N, V, diffRough, emissMetl);
	}

	FragColor = vec4(finalColor + emissMetl.rgb, 1.0);
}