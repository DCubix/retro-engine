#pragma once

#include "shader.h"

enum class BlendFunction {
	one = GL_ONE,
	zero = GL_ZERO,
	srcColor = GL_SRC_COLOR,
	oneMinusSrcColor = GL_ONE_MINUS_SRC_COLOR,
	destinationColor = GL_DST_COLOR,
	oneMinusDestinationColor = GL_ONE_MINUS_DST_COLOR,
	srcAlpha = GL_SRC_ALPHA,
	oneMinusSrcAlpha = GL_ONE_MINUS_SRC_ALPHA,
	destinationAlpha = GL_DST_ALPHA,
	oneMinusDestinationAlpha = GL_ONE_MINUS_DST_ALPHA
};

enum class BlendEquation {
	add = GL_FUNC_ADD,
	subtract = GL_FUNC_SUBTRACT,
	reverseSubtract = GL_FUNC_REVERSE_SUBTRACT,
	min = GL_MIN,
	max = GL_MAX
};

struct BlendState {
	bool enabled{ false };
	BlendFunction source{ BlendFunction::one };
	BlendFunction destination{ BlendFunction::zero };
	BlendEquation equation{ BlendEquation::add };

	void Apply() const;
};

enum class CompareFunction {
	never = GL_NEVER,
	less = GL_LESS,
	equal = GL_EQUAL,
	lessEqual = GL_LEQUAL,
	greater = GL_GREATER,
	notEqual = GL_NOTEQUAL,
	greaterEqual = GL_GEQUAL,
	always = GL_ALWAYS
};

enum class StencilFunction {
	never = GL_NEVER,
	less = GL_LESS,
	equal = GL_EQUAL,
	lessEqual = GL_LEQUAL,
	greater = GL_GREATER,
	notEqual = GL_NOTEQUAL,
	greaterEqual = GL_GEQUAL,
	always = GL_ALWAYS
};

enum class StencilOperation {
	keep = GL_KEEP,
	zero = GL_ZERO,
	replace = GL_REPLACE,
	increment = GL_INCR,
	decrement = GL_DECR,
	invert = GL_INVERT,
	incrementWrap = GL_INCR_WRAP,
	decrementWrap = GL_DECR_WRAP
};

struct DepthStencilState {
	struct {
		bool enabled{ false };
		CompareFunction function{ CompareFunction::less };

		bool write{ true };
		float clearValue{ 1.0f };

		float near{ 0.0f };
		float far{ 1.0f };
	} depth;

	struct {
		bool enabled{ false };
		StencilFunction function{ StencilFunction::always };
		u32 reference{ 0 }; // controls glStencilFunc reference value
		u32 mask{ 0xFF }; // controls glStencilFunc value

		u32 writeMask{ 0xFF }; // controls glStencilMask value

		StencilOperation fail{ StencilOperation::keep };
		StencilOperation depthFail{ StencilOperation::keep };
		StencilOperation depthPass{ StencilOperation::keep };

		u32 clearValue{ 0 }; // controls glClearStencil value
	} stencil;

	void Apply() const;
};

enum class CullMode {
	none = GL_NONE,
	front = GL_FRONT,
	back = GL_BACK,
	both = GL_FRONT_AND_BACK
};

enum class WindingOrder {
	clockwise = GL_CW,
	counterClockwise = GL_CCW
};

enum class PolygonMode {
	point = GL_POINT,
	line = GL_LINE,
	fill = GL_FILL
};

struct RasterizerState {
	struct {
		CullMode cullMode{ CullMode::back };
		WindingOrder windingOrder{ WindingOrder::counterClockwise };
		PolygonMode polygonMode{ PolygonMode::fill };
	} geometry;
	
	struct {
		bool enabled{ false };
		u32 box[4]{ 0, 0, 0, 0 }; // left, right, bottom, top
	} scissorTest;

	struct {
		bool enabled{ false };
		float factor{ 0.0f };
		float units{ 0.0f };
	} depthBias;

	void Apply() const;
};

struct PipelineDescription {
	Shader* shader{ nullptr };
	BlendState blendState{};
	DepthStencilState depthStencilState{};
	RasterizerState rasterizerState{};
};

class Pipeline {
public:
	Pipeline() = default;
	Pipeline(const PipelineDescription& description);

	void Bind() const;
	void Unbind() const;
private:
	Shader* mShader{ nullptr };
	BlendState mBlendState{};
	DepthStencilState mDepthStencilState{};
	RasterizerState mRasterizerState{};
};