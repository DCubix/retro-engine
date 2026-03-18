#version 460 core

// quad vertices array
vec2 quadVertices[4] = vec2[](
	vec2(0.0, 1.0),
	vec2(0.0, 0.0),
	vec2(1.0, 1.0),
	vec2(1.0, 0.0)
);

uniform vec2 uScale = vec2(1.0, 1.0);
uniform vec2 uOffset = vec2(0.0, 0.0);

out vec2 vTexCoord;

void main() {
	vec2 ndcPos = (quadVertices[gl_VertexID] * uScale) * 2.0 + uOffset;
	gl_Position = vec4(ndcPos - 1.0, 0.0, 1.0);
	vTexCoord = quadVertices[gl_VertexID];
}