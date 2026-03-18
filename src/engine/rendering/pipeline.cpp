#include "pipeline.h"

void BlendState::Apply() const  
{  
	if (enabled) {  
		glEnable(GL_BLEND);  
		glBlendFunc(static_cast<GLenum>(source), static_cast<GLenum>(destination));  
		glBlendEquation(static_cast<GLenum>(equation));  
	}  
	else {  
		glDisable(GL_BLEND);  
	}  
}

void DepthStencilState::Apply() const
{
	if (depth.enabled) {
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(static_cast<GLenum>(depth.function));
		glDepthMask(depth.write);
		glClearDepth(depth.clearValue);
		glDepthRange(depth.near, depth.far);
	}
	else {
		glDisable(GL_DEPTH_TEST);
	}
	
	if (stencil.enabled) {
		glEnable(GL_STENCIL_TEST);
		glStencilFunc(static_cast<GLenum>(stencil.function), stencil.reference, stencil.mask);
		glStencilOp(static_cast<GLenum>(stencil.fail), static_cast<GLenum>(stencil.depthFail), static_cast<GLenum>(stencil.depthPass));
		glStencilMask(stencil.writeMask);
		glClearStencil(stencil.clearValue);
	}
	else {
		glDisable(GL_STENCIL_TEST);
	}
}

void RasterizerState::Apply() const
{
	if (geometry.cullMode == CullMode::none) {
		glDisable(GL_CULL_FACE);
	}
	else {
		glEnable(GL_CULL_FACE);
	}

	switch (geometry.cullMode) {
		case CullMode::front:
			glCullFace(GL_FRONT);
			break;
		case CullMode::back:
			glCullFace(GL_BACK);
			break;
		case CullMode::both:
			glCullFace(GL_FRONT_AND_BACK);
			break;
		default: break;
	};

	if (geometry.windingOrder == WindingOrder::clockwise) {
		glFrontFace(GL_CW);
	}
	else {
		glFrontFace(GL_CCW);
	}

	switch (geometry.polygonMode) {
		case PolygonMode::point:
			glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
			break;
		case PolygonMode::line:
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			break;
		case PolygonMode::fill:
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			break;
		default: break;
	};

	if (depthBias.enabled) {
		glEnable(GL_POLYGON_OFFSET_FILL);
		glPolygonOffset(depthBias.factor, depthBias.units);
	}
	else {
		glDisable(GL_POLYGON_OFFSET_FILL);
	}

	if (scissorTest.enabled) {
		glEnable(GL_SCISSOR_TEST);
		glScissor(scissorTest.box[0], scissorTest.box[1], scissorTest.box[2], scissorTest.box[3]);
	}
	else {
		glDisable(GL_SCISSOR_TEST);
	}

	glEnable(GL_FRAMEBUFFER_SRGB);
}

Pipeline::Pipeline(const PipelineDescription& description)
{
	mShader = description.shader;
	mBlendState = description.blendState;
	mDepthStencilState = description.depthStencilState;
	mRasterizerState = description.rasterizerState;
}

void Pipeline::Bind() const
{
	if (mShader) {
		mShader->Bind();
	}
	mBlendState.Apply();
	mDepthStencilState.Apply();
	mRasterizerState.Apply();
}

void Pipeline::Unbind() const
{
	if (mShader) {
		mShader->Unbind();
	}
	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_STENCIL_TEST);
	glDisable(GL_CULL_FACE);
	glDisable(GL_SCISSOR_TEST);
	glDisable(GL_POLYGON_OFFSET_FILL);
	//glDisable(GL_FRAMEBUFFER_SRGB);
}
