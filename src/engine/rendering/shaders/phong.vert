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

const vec3 lightPos = vec3(5.0, 5.0, 5.0);

out DATA {
	vec3 position;
	vec3 normal;
	vec3 tangent;
	vec2 texCoord;
	vec4 color;
	
	// tangent space 
	vec3 tangentPosition;
	vec3 tangentViewPosition;
	vec3 tangentLightPosition;
} vout;

uniform mat4 uProjectionMatrix;
uniform mat4 uViewMatrix;
uniform vec3 uViewPosition;

#ifndef IS_INSTANCED
uniform mat4 uModelMatrix;
#endif

void main()
{
#ifdef IS_INSTANCED
	// Use the instance model matrix for instanced rendering
	mat4 modelMatrix = vModelMatrix;
#else
	mat4 modelMatrix = uModelMatrix;
#endif
	vec4 pos = modelMatrix * vec4(vPosition, 1.0);
	gl_Position = uProjectionMatrix * uViewMatrix * pos;

	// Pass the normal to the fragment shader
	mat3 normalMatrix = transpose(inverse(mat3(uModelMatrix)));
	vout.normal = normalize(normalMatrix * vNormal);
	vout.tangent = normalize(normalMatrix * vTangent);
	vout.tangent = normalize(vout.tangent - dot(vout.tangent, vout.normal) * vout.normal);
	vec3 bitangent = normalize(cross(vout.normal, vout.tangent));

	if (dot(cross(vout.normal, vout.tangent), bitangent) < 0.0) {
		vout.tangent *= -1.0;
	}
	
	vout.position = vec3(pos);
	vout.texCoord = vTexCoord;
	vout.color = vColor;

	mat3 tbn = transpose(mat3(vout.tangent, bitangent, vout.normal));

	vout.tangentPosition = tbn * vout.position;
	vout.tangentViewPosition = tbn * uViewPosition;
	vout.tangentLightPosition = tbn * lightPos;
}