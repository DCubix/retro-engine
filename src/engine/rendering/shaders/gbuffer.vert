#version 460 core
layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec3 vTangent;
layout (location = 3) in vec2 vTexCoord;
layout (location = 4) in vec4 vColor;
// layout (location = 5) in ivec4 vBoneIDs;
// layout (location = 6) in vec4 vBoneWeights;
#ifdef IS_INSTANCED
layout (location = 7) in uint vObjectID;
layout (location = 8) in mat4 vModelMatrix;
#endif

struct ShaderOutput {
	vec4 position;
	vec3 normal;
	vec3 tangent;
	vec2 texCoord;
	vec4 vertexColor;
};

struct InputData {
	vec3 position;
	vec3 normal;
	vec3 tangent;
	vec2 texCoord;
	vec4 vertexColor;
};

out DATA {
	vec3 position;
	vec3 tangentPosition;
	vec3 tangentViewPosition;
	vec3 viewPosition;
	vec3 normal;
	vec3 tangent;
	vec2 texCoord;
	vec4 vertexColor;
	mat3 tbn;
} vout;

uniform mat4 uProjectionMatrix;
uniform mat4 uViewMatrix;
uniform vec3 uViewPosition;

#ifndef IS_INSTANCED
uniform mat4 uModelMatrix;
#define U_MODEL_MATRIX uModelMatrix
#else
#define U_MODEL_MATRIX vModelMatrix
#endif

#define U_PROJ_MATRIX uProjectionMatrix
#define U_VIEW_MATRIX uViewMatrix

vec4 WorldTransform(vec4 position)
{
	return U_MODEL_MATRIX * position;
}

vec4 ViewProjectionTransform(vec4 position)
{
	return U_PROJ_MATRIX * U_VIEW_MATRIX * position;
}

vec3 NormalTransform(vec3 vector)
{
	mat3 normalMatrix = transpose(inverse(mat3(U_MODEL_MATRIX)));
	return normalize(normalMatrix * vector);
}

vec3 TangentTransform(vec3 normal, vec3 tangent)
{
	vec3 T = NormalTransform(tangent);
	T = normalize(T - dot(T, normal) * normal);
	if (dot(cross(normal, tangent), T) < 0.0) {
		T *= -1.0;
	}
	return T;
}

ShaderOutput DefaultVertexShader(in InputData i)
{
	ShaderOutput o;
	o.position = WorldTransform(vec4(i.position, 1.0));
	o.normal = NormalTransform(i.normal);
	o.tangent = TangentTransform(i.normal, i.tangent);
	o.texCoord = i.texCoord;
	o.vertexColor = i.vertexColor;
	return o;
}

#ifndef CUSTOM_SHADER_VS
ShaderOutput VertexShader(in InputData i)
{
	return DefaultVertexShader(i);
}
#else
#include "custom_vertex_shader"
#endif

void main()
{
	InputData i;
	i.position = vPosition;
	i.normal = vNormal;
	i.tangent = vTangent;
	i.texCoord = vTexCoord;
	i.vertexColor = vColor;
	// i.boneIDs = vBoneIDs;
	// i.boneWeights = vBoneWeights;

	ShaderOutput o = VertexShader(i);

	vec3 bitangent = normalize(cross(o.normal, o.tangent));

	vout.position = o.position.xyz;
	vout.normal = o.normal;
	vout.tangent = o.tangent;
	vout.texCoord = o.texCoord;
	vout.vertexColor = o.vertexColor;

	vout.tbn = mat3(o.tangent, bitangent, o.normal);

	mat3 tbn = transpose(vout.tbn);
	vout.tangentPosition = tbn * vout.position;
	vout.tangentViewPosition = tbn * uViewPosition;
	vout.viewPosition = uViewPosition;

	gl_Position = ViewProjectionTransform(o.position);
}