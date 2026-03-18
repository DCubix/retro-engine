#version 430 core
#extension GL_ARB_bindless_texture : require

#define PI 3.14159265

// Blinn-Phong lighting model based on roughness/metallic workflow
#define MIN_POWER 1.0
#define MAX_POWER 256.0
#define MAX_INTENSITY 0.9

#define STEP_AMBIENT 0
#define STEP_DIRECT_LIGHTING 1
#define STEP_EMISSIVE 2

const mat4 biasMatrix = mat4(
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 0.5, 0.0,
	0.5, 0.5, 0.5, 1.0
);

const vec2 PoissonDisk[64] = vec2[](
	vec2(-0.613392, 0.617481),
	vec2(0.170019, -0.040254),
	vec2(-0.299417, 0.791925),
	vec2(0.645680, 0.493210),
	vec2(-0.651784, 0.717887),
	vec2(0.421003, 0.027070),
	vec2(-0.817194, -0.271096),
	vec2(-0.705374, -0.668203),
	vec2(0.977050, -0.108615),
	vec2(0.063326, 0.142369),
	vec2(0.203528, 0.214331),
	vec2(-0.667531, 0.326090),
	vec2(-0.098422, -0.295755),
	vec2(-0.885922, 0.215369),
	vec2(0.566637, 0.605213),
	vec2(0.039766, -0.396100),
	vec2(0.751946, 0.453352),
	vec2(0.078707, -0.715323),
	vec2(-0.075838, -0.529344),
	vec2(0.724479, -0.580798),
	vec2(0.222999, -0.215125),
	vec2(-0.467574, -0.405438),
	vec2(-0.248268, -0.814753),
	vec2(0.354411, -0.887570),
	vec2(0.175817, 0.382366),
	vec2(0.487472, -0.063082),
	vec2(-0.084078, 0.898312),
	vec2(0.488876, -0.783441),
	vec2(0.470016, 0.217933),
	vec2(-0.696890, -0.549791),
	vec2(-0.149693, 0.605762),
	vec2(0.034211, 0.979980),
	vec2(0.503098, -0.308878),
	vec2(-0.016205, -0.872921),
	vec2(0.385784, -0.393902),
	vec2(-0.146886, -0.859249),
	vec2(0.643361, 0.164098),
	vec2(0.634388, -0.049471),
	vec2(-0.688894, 0.007843),
	vec2(0.464034, -0.188818),
	vec2(-0.440840, 0.137486),
	vec2(0.364483, 0.511704),
	vec2(0.034028, 0.325968),
	vec2(0.099094, -0.308023),
	vec2(0.693960, -0.366253),
	vec2(0.678884, -0.204688),
	vec2(0.001801, 0.780328),
	vec2(0.145177, -0.898984),
	vec2(0.062655, -0.611866),
	vec2(0.315226, -0.604297),
	vec2(-0.780145, 0.486251),
	vec2(-0.371868, 0.882138),
	vec2(0.200476, 0.494430),
	vec2(-0.494552, -0.711051),
	vec2(0.612476, 0.705252),
	vec2(-0.578845, -0.768792),
	vec2(-0.772454, -0.090976),
	vec2(0.504440, 0.372295),
	vec2(0.155736, 0.065157),
	vec2(0.391522, 0.849605),
	vec2(-0.620106, -0.328104),
	vec2(0.789239, -0.419965),
	vec2(-0.545396, 0.538133),
	vec2(-0.178564, -0.596057)
);

out vec4 FragColor;
in vec2 vTexCoord;

layout (bindless_sampler) uniform sampler2D ugbDiffuseRoughness;
layout (bindless_sampler) uniform sampler2D ubgEmissiveMetallic;
layout (bindless_sampler) uniform sampler2D ugbPosition;
layout (bindless_sampler) uniform sampler2D ugbNormal;

layout (bindless_sampler) uniform sampler2D uShadowMap;
layout (bindless_sampler) uniform samplerCube uShadowCubeMap;

struct Light {
	vec4 colorIntensity;
	vec4 positionRadius;
	vec4 directionCutoffAngle;
	int lightType;
	float _padding0;
	float _padding1;
	float _padding2;
};

uniform uint uLightStep;

layout(std140, binding = 2) uniform LightUBO {
	Light uCurrentLight;
};

uniform bool uShadowEnabled;
uniform vec3 uViewPosition;
uniform mat4 uLightViewProjection;
uniform float uShadowFar;

const float uShadowNear = 0.01;

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

float PCF(vec3 coord, float bias, float radius) {
	if (coord.z > 1.0) {
		return 0.0;
	}

	float fd = coord.z - bias;
	float sum = 0.0;

	for (int i = 0; i < 64; i++) {
		vec2 uv = coord.xy + PoissonDisk[i] * radius;
		float z = texture(uShadowMap, uv).r;
		sum += z < fd ? 0.0 : 1.0;
	}
	return sum / 64.0;
}

float CalculateDirectionalShadows(vec3 P, float NdotL) {
	if (!uShadowEnabled) {
		return 1.0;
	}

	vec4 shadowCoord = biasMatrix * uLightViewProjection * vec4(P, 1.0);
	vec3 coord = shadowCoord.xyz / shadowCoord.w;

	float bias = 0.000001 * tan(acos(NdotL));
	bias = clamp(bias, 0.0, 0.01);

	return PCF(coord, bias, 0.007);
}

float CalculatePointShadows(Light light, vec3 P, vec3 Vp, float NdotL) {
	if (!uShadowEnabled) {
		return 1.0;
	}

	vec3 lightPos = light.positionRadius.xyz;
	vec3 fragToLight = P - lightPos;

	float closestDepth = texture(uShadowCubeMap, fragToLight).r;
	closestDepth *= uShadowFar;

	float currentDepth = length(fragToLight);

	float bias = 0.05 * tan(acos(NdotL));
	bias = clamp(bias, 0.0, 0.05);

	float viewDist = length(P - Vp);
	float diskRadius = (1.0 + (viewDist / uShadowFar)) / 25.0;
	float shadow = 0.0;
	for (int i = 0; i < 64; i++) {
		vec3 samplePos = fragToLight + PoissonDisk[i].xyy * diskRadius;
		float sampleDepth = texture(uShadowCubeMap, samplePos).r;
		sampleDepth *= uShadowFar;

		if (currentDepth - bias > sampleDepth) {
			shadow += 1.0;
		}
	}
	return 1.0 - (shadow / 64.0);
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

	float NoL = max(dot(N, L), 0.0);
	float NoV = max(dot(N, V), 0.0);

	// Oren-Nayar diffuse model
	float lightFactor = OrenNayarDiffuse(L, V, N, roughness, 0.8);
	//

	lightFactor *= attenuation;

	if (light.lightType == 1) {
		lightFactor *= CalculatePointShadows(light, P, uViewPosition, NoL);
	} else {
		lightFactor *= CalculateDirectionalShadows(P, NoL);
	}

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
	float rimFactor = clamp(pow(1.0 - NoV, 5.0), 0.0, 1.0);
	
	// rim color is light color slightly desaturated
	vec3 rimLighting = lightColor * rimFactor * lightIntensity * lightFactor;

	vec3 finalColor = (
		(diffuse * diffuseColor) +
		(specular * specularColor) +
		rimLighting 
	);

	return finalColor;
}

void main() {
	vec4 diffRough = texture(ugbDiffuseRoughness, vTexCoord);
	switch (uLightStep) {
		case STEP_AMBIENT:
			FragColor = vec4(uAmbientColor * diffRough.rgb, 1.0);
			return;
		case STEP_DIRECT_LIGHTING: {
			vec3 P = texture(ugbPosition, vTexCoord).xyz;
			vec3 N = texture(ugbNormal, vTexCoord).xyz;
			vec3 V = normalize(uViewPosition - P);
			vec4 emissMetl = texture(ubgEmissiveMetallic, vTexCoord);
			FragColor = vec4(CalculateLighting(uCurrentLight, P, N, V, diffRough, emissMetl), 1.0);
		} break;
		case STEP_EMISSIVE: {
			vec4 emissMetl = texture(ubgEmissiveMetallic, vTexCoord);
			FragColor = vec4(emissMetl.rgb, 1.0);
		} return;
	}
}