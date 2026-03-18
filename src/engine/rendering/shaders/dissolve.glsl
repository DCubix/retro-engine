[[fragment_shader]]

layout (location = 0, bindless_sampler) uniform sampler2D uNoise;
uniform float uAmount;

const float glowRange = 0.03;
const float glowFalloff = 0.05;
const vec3 glowColor = vec3(1.0, 0.3, 0.0);

ShaderOutput FragmentShader(in InputData i) {
	ShaderOutput o = DefaultFragmentShader(i);
	
	float dissolve = SampleTriplanar(uNoise, i.position, i.normal).r;
	float isVisible = dissolve - uAmount;

	if (isVisible < 0.0) discard;

	float isGlowing = smoothstep(glowRange + glowFalloff, glowRange, isVisible);
	vec3 glow = isGlowing * glowColor;

	o.emissive += glow;

	return o;
}