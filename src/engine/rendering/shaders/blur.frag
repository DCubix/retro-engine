#version 460 core
#extension GL_ARB_bindless_texture : require

out vec4 FragColor;
in vec2 vTexCoord;

uniform vec2 uResolution;
uniform vec2 uDirection;
uniform float uLod;

layout(std430, binding = 0) buffer AllTextures {
	sampler2D uChannels[];
};

// Filter from https://github.com/Experience-Monks/glsl-fast-gaussian-blur
vec4 blur13(sampler2D image, vec2 uv, vec2 resolution, vec2 direction) {
	vec4 color = vec4(0.0);
	vec2 off1 = vec2(1.411764705882353) * direction;
	vec2 off2 = vec2(3.2941176470588234) * direction;
	vec2 off3 = vec2(5.176470588235294) * direction;
	color += vec4(textureLod(image, uv, uLod).rgb, 1.0) * 0.1964825501511404;
	color += vec4(textureLod(image, uv + (off1 / resolution), uLod).rgb, 1.0) * 0.2969069646728344;
	color += vec4(textureLod(image, uv - (off1 / resolution), uLod).rgb, 1.0) * 0.2969069646728344;
	color += vec4(textureLod(image, uv + (off2 / resolution), uLod).rgb, 1.0) * 0.09447039785044732;
	color += vec4(textureLod(image, uv - (off2 / resolution), uLod).rgb, 1.0) * 0.09447039785044732;
	color += vec4(textureLod(image, uv + (off3 / resolution), uLod).rgb, 1.0) * 0.010381362401148057;
	color += vec4(textureLod(image, uv - (off3 / resolution), uLod).rgb, 1.0) * 0.010381362401148057;
	return color;
}

void main() {
	FragColor = blur13(uChannels[0], vTexCoord, uResolution, uDirection);
}