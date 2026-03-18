#version 460 core
in vec4 FragPos;

uniform vec3 uLightPosition;
uniform float uFar;

void main() {
	float dist = length(FragPos.xyz - uLightPosition);
	dist /= uFar;
	gl_FragDepth = dist;
}