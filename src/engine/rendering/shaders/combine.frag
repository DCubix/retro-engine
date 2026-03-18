#version 460 core
#extension GL_ARB_bindless_texture : require

out vec4 FragColor;
in vec2 vTexCoord;

uniform vec2 uResolution;

uniform uint uMode;

layout(std430, binding = 0) buffer AllTextures {
	sampler2D uChannels[];
};

void main() {
	vec4 a = texture(uChannels[0], vTexCoord);
	vec4 b = texture(uChannels[1], vTexCoord);

	if (uMode == 0) { // Add
		FragColor = a + b;
	} else { // Multiply
		FragColor = a * b;
	}
}