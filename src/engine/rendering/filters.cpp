#include "filter.h"

#include "shaders/generated/quad_vert.h"
#include "shaders/generated/blur_frag.h"
#include "shaders/generated/combine_frag.h"

#include "filters.h"

#include <fmt/format.h>

Filter::Filter(const str& src)
	: Shader(ShaderDescription{
		.shaderSources = {
			{ ShaderType::vertexShader, quad_vert_src },
			{ ShaderType::fragmentShader, src },
		},
	})
{}

BlurFilter::BlurFilter()
	: Filter(blur_frag_src)
{}

void BlurFilter::OnSetup()
{
	mHorizontal = !mHorizontal;
	if (mHorizontal) {
		SetUniform("uDirection", Vector2{ 1.0f, 0.0f });
	}
	else {
		SetUniform("uDirection", Vector2{ 0.0f, 1.0f });
	}
	SetUniform("uLod", mLod);
}

void BlurFilter::Reset()
{
	mHorizontal = false;
}

CombineFilter::CombineFilter()
	: Filter(combine_frag_src)
{}

void CombineFilter::OnSetup()
{
	SetUniform("uMode", static_cast<u32>(mCombineMode));
}