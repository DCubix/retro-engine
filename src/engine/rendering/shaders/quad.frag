#version 460 core
#extension GL_ARB_bindless_texture : require

layout (bindless_sampler) uniform sampler2D uTexture;

in vec2 vTexCoord;
out vec4 FragColor;

uniform float uNear;
uniform float uFar;
uniform bool uIsDepth;

float LinearizeDepth(float depth)
{
    return uNear * uFar / (uFar - depth * (uFar - uNear));
}

void main() {
	if (!uIsDepth) {
		FragColor = texture(uTexture, vTexCoord);
	} else {
		float depthValue = texture(uTexture, vTexCoord).r;
		float linearDepth = LinearizeDepth(depthValue);
		float normalizedDepth = (linearDepth - uNear) / (uFar - uNear);
		FragColor = vec4(vec3(normalizedDepth), 1.0);
	}
}