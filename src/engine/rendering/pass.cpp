#include "pass.h"

Pass::Pass(const PassDescription& description)
	: mFrameBuffer(description.frameBuffer),
	mClearColor(description.clearColor),
	mClearColorEnabled(description.clear.color),
	mClearDepthEnabled(description.clear.depth),
	mClearStencilEnabled(description.clear.stencil),
	mOverrideViewport(description.overrideViewport) {
	std::copy(std::begin(description.viewport), std::end(description.viewport), std::begin(mViewport));
}

void Pass::Begin()
{
	if (mFrameBuffer) {
		mFrameBuffer->Bind();
	}
	else {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
	if (mOverrideViewport) {
		glViewport(mViewport[0], mViewport[1], mViewport[2], mViewport[3]);
	}

	GLenum clearFlags = 0;
	if (mClearColorEnabled) {
		glClearColor(mClearColor.x, mClearColor.y, mClearColor.z, mClearColor.w);
		clearFlags |= GL_COLOR_BUFFER_BIT;
	}
	if (mClearDepthEnabled) {
		clearFlags |= GL_DEPTH_BUFFER_BIT;
	}
	if (mClearStencilEnabled) {
		clearFlags |= GL_STENCIL_BUFFER_BIT;
	}
	
	if (clearFlags) {
		glClear(clearFlags);
	}
}

void Pass::End()
{
	if (mFrameBuffer) {
		mFrameBuffer->Unbind();
	}
}
