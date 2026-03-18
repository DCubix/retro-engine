#version 460 core
layout (location = 0) in vec3 vPosition;
// layout (location = 5) in ivec4 vBoneIDs;
// layout (location = 6) in vec4 vBoneWeights;
#ifdef IS_INSTANCED
layout (location = 7) in uint vObjectID;
layout (location = 8) in mat4 vModelMatrix;
#endif

#ifndef IS_INSTANCED
uniform mat4 uModelMatrix;
#define U_MODEL_MATRIX uModelMatrix
#else
#define U_MODEL_MATRIX vModelMatrix
#endif

void main() {
	gl_Position = U_MODEL_MATRIX * vec4(vPosition, 1.0);
}