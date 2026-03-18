#pragma once

#include "framebuffer.h"
#include "../external/raymath/raymath.h"

struct RENGINE_API PassDescription {
	FrameBuffer* frameBuffer{ nullptr }; // nullptr = default framebuffer
	
	Vector4 clearColor{ 0.0f, 0.0f, 0.0f, 1.0f };

	struct {
		bool color{ true };
		bool depth{ false };
		bool stencil{ false };
	} clear;

	bool overrideViewport{ false };
	u32 viewport[4]{ 0 };
};

class RENGINE_API Pass {
public:
	Pass() = default;
	Pass(const PassDescription& description);

	// default copy and assign
	Pass(const Pass&) = default;
	Pass& operator=(const Pass&) = default;

	// default move and assign
	Pass(Pass&&) = default;
	Pass& operator=(Pass&&) = default;

	void Begin();
	void End();

private:
	FrameBuffer* mFrameBuffer{ nullptr };
	Vector4 mClearColor{ 0.0f, 0.0f, 0.0f, 1.0f };
	bool mClearColorEnabled{ false };
	bool mClearDepthEnabled{ false };
	bool mClearStencilEnabled{ false };
	bool mOverrideViewport{ false };
	u32 mViewport[4]{ 0 };
};