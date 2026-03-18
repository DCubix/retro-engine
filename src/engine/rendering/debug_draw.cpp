#include "debug_draw.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <unordered_map>

#include "font_data.h"
#include "../external/stb_image/stb_image.h"
#include "../utils/logger.h"

constexpr u32 primitiveRestartIndex = 0xFFFFFFFF;
constexpr u32 minSegmentCount = 3;

DebugDraw::DebugDraw(u32 windowWidth, u32 windowHeight)
	: mWindowWidth(windowWidth), mWindowHeight(windowHeight) {
	const str vert = R"(#version 460 core
layout (location = 0) in vec3 iPosition;
layout (location = 1) in vec2 iUV;
layout (location = 2) in vec4 iColor;
layout (location = 3) in float iPointSize;

out vec4 vColor;
out vec2 vUV;
flat out vec3 vStartPosition;
out vec3 vPosition;

uniform mat4 uMVP;

void main()
{
	vec4 pos       = uMVP * vec4(iPosition, 1.0);
    gl_Position    = pos;
    gl_PointSize   = iPointSize;
    vColor         = iColor;
    vUV            = iUV;
	vPosition      = pos.xyz / pos.w;
	vStartPosition = vPosition;
})";

	const str frag = R"(#version 460 core
#extension GL_ARB_bindless_texture : require

layout (bindless_sampler) uniform sampler2D uTexture;
uniform bool uHasTexture;

uniform bool uIsPoint;
uniform bool uPointSmooth;
uniform bool uIsLine;
uniform bool uIsText;
uniform vec2 uResolution;
uniform uint uStipplePattern;
uniform float uStippleFactor;

in vec4 vColor;
in vec2 vUV;
flat in vec3 vStartPosition;
in vec3 vPosition;

out vec4 oFragColor;  

#define FOG_DENSITY 0.035

float fogFactorExp2(
  const float dist,
  const float density
) {
  const float LOG2 = -1.442695;
  float d = density * dist;
  return 1.0 - clamp(exp2(d * d * LOG2), 0.0, 1.0);
}

void main()  
{
	float factor = 1.0;
	if (uIsPoint) {
		if (!uPointSmooth) {
			vec2 coord = gl_PointCoord.xy - vec2(0.5);
			if (length(coord) > 0.5) {
				discard;
			}
		}
		else {
			vec2 coord = gl_PointCoord.xy - vec2(0.5);
			float circle = length(coord);
			factor = 1.0 - smoothstep(0.0, 0.5, circle);
			if (circle > 0.5) {
				discard;
			}
		}
	}

	if (uIsLine) {
		vec2 dir = (vPosition.xy - vStartPosition.xy) * uResolution / 2.0;
		float dist = length(dir);

		uint bit = uint(round(dist / uStippleFactor)) & 15U;
		if ((uStipplePattern & (1U << bit)) == 0U) {
			discard;
		}
	}

	float fogDist = gl_FragCoord.z / gl_FragCoord.w;
	float fogFactor = fogFactorExp2(fogDist, FOG_DENSITY);
    
	vec4 col = vColor;
	if (uHasTexture) {
		vec4 tex = texture(uTexture, vUV);
		if (uIsText) {
			if (tex.r < 0.5) {
				discard;
			}
		}
		else {
			col *= tex;
		}
	}
	oFragColor = vec4(col.rgb, col.a * factor * (1.0 - fogFactor));
})";

	ShaderDescription shaderDesc{
		.shaderSources = {
			{ ShaderType::vertexShader, vert },
			{ ShaderType::fragmentShader, frag }
		},
	};
	mShader = std::make_unique<Shader>(shaderDesc);

	VertexFormat vertexFormat{
		VertexAttribute{ DataType::float3Value, false }, // position
		VertexAttribute{ DataType::float2Value, false }, // tex coords
		VertexAttribute{ DataType::float4Value, false }, // color
		VertexAttribute{ DataType::floatValue, false }   // point size
	};

	VertexBufferDescription<DebugDrawVertex> vboDesc{
		.format = vertexFormat,
		.usage = BufferUsage::dynamicDraw,
		.initialData = nullptr,
		.initialDataLength = 60000
	};

	mVAO = VertexArray();
	mVAO.Bind();
	mVBO = VertexBuffer(vboDesc);
	mVBO.Bind();
	mVAO.Unbind();

	{
		int w, h, comp;
		byte* data = stbi_load_from_memory(font_png, font_png_len, &w, &h, &comp, 4);
		if (!data) {
			utils::LogE("Failed to load font texture");
			return;
		}

		for (u32 y = 0; y < 16; y++) {
			for (u32 x = 0; x < 16; x++) {
				u32 index = y * 16 + x;
				if (index >= 256) break;

				u32 tileX = x * 16;
				u32 tileY = y * 16;
				u32 offset = (w * tileY + tileX) * comp;
				byte value = data[offset + 1];
				mFontCharWidths[index] = u32(f32(value) / 255.0f * 16.0f);
			}
		}

		TextureDescription texDesc{
			.type = TextureType::texture2D,
			.width = u32(w),
			.height = u32(h),
			.samplerDescription = {
				.wrapS = TextureWrapMode::clampToEdge,
				.wrapT = TextureWrapMode::clampToEdge,
				.minFilter = TextureFilterMode::nearest,
				.magFilter = TextureFilterMode::nearest,
			},
			.layers = { data, nullptr, nullptr, nullptr, nullptr, nullptr },
			.format = TextureFormat::RGBA,
		};
		mFontTexture = std::make_unique<Texture>(texDesc);
	}

	glEnable(GL_PROGRAM_POINT_SIZE);
	glEnable(GL_PRIMITIVE_RESTART);
	glPrimitiveRestartIndex(primitiveRestartIndex);
}

void DebugDraw::Begin(const Vector3& viewPositon)
{
	if (!mEnded) return;
	mViewPosition = viewPositon;
	mCommands.clear();
	mBatches.clear();
	mEnded = false;
}

void DebugDraw::End(const Matrix& mvp)
{
	const Matrix twoDProjection = MatrixOrtho(
		0.0f, static_cast<f32>(mWindowWidth),
		static_cast<f32>(mWindowHeight), 0.0f,
		-1.0f, 1.0f
	);

	BuildBatches();

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	mShader->Bind();
	mVAO.Bind();

	mShader->SetUniform("uResolution", Vector2{ static_cast<f32>(mWindowWidth), static_cast<f32>(mWindowHeight) });

	for (auto& batch : mBatches) {
		if (batch.twoD) {
			glDisable(GL_CULL_FACE);
			glDisable(GL_DEPTH_TEST);
		}
		else {
			glEnable(GL_CULL_FACE);
			if (batch.depthEnabled) {
				glEnable(GL_DEPTH_TEST);
			}
			else {
				glDisable(GL_DEPTH_TEST);
			}
		}

		mShader->SetUniform("uIsPoint", batch.primitiveType == DebugDrawPrimitiveType::point);
		mShader->SetUniform("uIsLine", batch.primitiveType == DebugDrawPrimitiveType::line);
		mShader->SetUniform("uIsText", batch.isChar);
		mShader->SetUniform("uPointSmooth", batch.smoothPoints);
		mShader->SetUniform("uStipplePattern", batch.stipplePattern);
		mShader->SetUniform("uStippleFactor", batch.stippleFactor);

		mShader->SetUniform("uHasTexture", batch.texture != nullptr);
		if (batch.texture) {
			mShader->SetUniformTextureHandle("uTexture", batch.texture->GetBindlessHandle());
		}

		mShader->SetUniform("uMVP", batch.twoD ? twoDProjection : mvp);

		GLenum primitiveType = GL_TRIANGLES;
		switch (batch.primitiveType) {
			case DebugDrawPrimitiveType::point: primitiveType = GL_POINTS; break;
			case DebugDrawPrimitiveType::line: primitiveType = GL_LINES; break;
			case DebugDrawPrimitiveType::triangle: primitiveType = GL_TRIANGLES; break;
			case DebugDrawPrimitiveType::triangleFan: primitiveType = GL_TRIANGLE_FAN; break;
		}
		
		glDrawArrays(
			primitiveType,
			batch.offset,
			batch.count
		);
	}

	mVAO.Unbind();

	mEnded = true;
}

static Vector3 cubeVertices[8] = {
	{-0.5f, -0.5f, -0.5f},
	{ 0.5f, -0.5f, -0.5f},
	{ 0.5f,  0.5f, -0.5f},
	{-0.5f,  0.5f, -0.5f},
	{-0.5f, -0.5f,  0.5f},
	{ 0.5f, -0.5f,  0.5f},
	{ 0.5f,  0.5f,  0.5f},
	{-0.5f,  0.5f,  0.5f}
};
static u32 cubeLineIndices[24] = {
	0, 1, 1, 2, 2, 3, 3, 0,
	4, 5, 5, 6, 6, 7, 7, 4,
	0, 4, 1, 5, 2, 6, 3, 7
};
static u32 cubeTriangleIndices[36] = {
	0, 1, 2, 2, 3, 0,
	4, 5, 6, 6, 7, 4,
	0, 1, 5, 5, 4, 0,
	2, 3, 7, 7, 6, 2,
	0, 3, 7, 7, 4, 0,
	1, 2, 6, 6, 5, 1
};

void DebugDraw::Point(const Vector3& position, const Vector4& color, float size, bool smooth)
{
	DebugDrawCommand cmd{};
	cmd.primitiveType = DebugDrawPrimitiveType::point;
	cmd.vertices.emplace_back(position, color, size);
	cmd.smoothPoints = smooth;
	PushCommand(cmd);
}

void DebugDraw::Line(const Vector3& start, const Vector3& end, const Vector4& color)
{
	Line(start, end, color, color);
}

void DebugDraw::LineLoop(const std::vector<Vector3>& points, const std::vector<Vector4>& colors)
{
	assert(points.size() == colors.size() && "Points and colors size mismatch");

	DebugDrawCommand cmd{};
	cmd.primitiveType = DebugDrawPrimitiveType::line;
	for (size_t i = 0; i < points.size(); i++) {
		cmd.vertices.emplace_back(points[i], colors[i]);
	}
	PushCommand(cmd);
}

void DebugDraw::LineLoop(const std::vector<Vector3>& points, const Vector4& color)
{
	DebugDrawCommand cmd{};
	cmd.primitiveType = DebugDrawPrimitiveType::line;
	for (size_t i = 0; i < points.size(); i++) {
		cmd.vertices.emplace_back(points[i], color);
	}
	PushCommand(cmd);
}

void DebugDraw::Circle(
	const Vector3& center,
	const Vector3& normal,
	const Vector3& up,
	float radius,
	const Vector4& color,
	u32 segmentsOverride
)
{
	// compute segments based on radius
	const u32 segments = segmentsOverride > 0
		? segmentsOverride
		: std::max(minSegmentCount, u32(std::floor(32 * std::sqrt(radius))));

	Vector3 n = Vector3Normalize(normal);
	Vector3 t = Vector3Normalize(up - n * Vector3DotProduct(n, up));
	Vector3 b = Vector3CrossProduct(n, t);

	DebugDrawCommand cmd{};
	cmd.primitiveType = DebugDrawPrimitiveType::line;

	auto fnGetCirclePoint = [&](u32 i) {
		f32 angle = (f32(i) / segments) * 2.0f * PI;
		return center + (t * ::cosf(angle) + b * ::sinf(angle)) * radius;
	};

	for (u32 i = 0; i < segments - 1; i++) {
		Vector3 start = fnGetCirclePoint(i);
		Vector3 end = fnGetCirclePoint(i + 1);

		Line(start, end, color);
	}

	// close the circle
	Vector3 start = fnGetCirclePoint(segments - 1);
	Vector3 end = fnGetCirclePoint(0);
	Line(start, end, color);

	PushCommand(cmd);
}

void DebugDraw::FillCircle(const Vector3& center, const Vector3& normal, const Vector3& up, float radius, const Vector4& color)
{
	// compute segments based on radius
	const u32 segments = std::max(minSegmentCount, u32(std::floor(32 * std::sqrt(radius))));

	Vector3 n = Vector3Normalize(normal);
	Vector3 t = Vector3Normalize(up - n * Vector3DotProduct(n, up));
	Vector3 b = Vector3CrossProduct(n, t);
	
	DebugDrawCommand cmd{};
	cmd.primitiveType = DebugDrawPrimitiveType::triangleFan;
	cmd.vertices.emplace_back(center, color);

	for (u32 i = 0; i < segments; i++) {
		f32 angle = f32(i) / segments * 2.0f * PI;
		Vector3 point = center +
			(t * ::cosf(angle) + b * ::sinf(angle)) * radius;
		cmd.vertices.emplace_back(point, color);
	}
	PushCommand(cmd);
}

void DebugDraw::Cube(const Vector3& center, const Vector3& size, const Vector4& color, const Matrix& transform)
{
	DebugDrawCommand cmd{};
	cmd.primitiveType = DebugDrawPrimitiveType::line;
	for (u32 i = 0; i < 24; i++) {
		Vector3 vertex = cubeVertices[cubeLineIndices[i]] * size;
		cmd.vertices.emplace_back(
			Vector3Transform(vertex, transform) + center,
			color
		);
	}
	PushCommand(cmd);
}

void DebugDraw::FillCube(const Vector3& center, const Vector3& size, const Vector4& color, const Matrix& transform)
{
	DebugDrawCommand cmd{};
	cmd.primitiveType = DebugDrawPrimitiveType::triangle;
	for (u32 i = 0; i < 36; i++) {
		Vector3 vertex = cubeVertices[cubeTriangleIndices[i]] * size;
		cmd.vertices.emplace_back(Vector3Transform(vertex, transform) + center, color);
	}
	PushCommand(cmd);
}

void DebugDraw::Sphere(const Vector3& center, float radius, const Vector4& color)
{
	// Wireframe sphere will be 3 circles in X Y and Z axis
	Circle(center, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, radius, color);
	Circle(center, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, radius, color);
	Circle(center, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f }, radius, color);
}

void DebugDraw::FillSphere(const Vector3& center, float radius, const Vector4& color)
{
	DebugDrawCommand cmd{};
	cmd.primitiveType = DebugDrawPrimitiveType::triangle;

	// TODO

	PushCommand(cmd);
}

void DebugDraw::Cone(
	const Vector3& center,
	const Vector3& direction,
	float radius, float height,
	const Vector4& color,
	bool reversed,
	bool silhouette
) {
	// Using Line
	const u32 segments = std::max(minSegmentCount, u32(std::floor(12 * std::sqrt(radius))));

	const Vector3 up = ::abs(direction.y) < 0.999f
		? Vector3{ 0.0f, 1.0f, 0.0f }
	: Vector3{ 1.0f, 0.0f, 0.0f };
	Vector3 n = Vector3Normalize(direction);
	Vector3 t = Vector3Normalize(up - n * Vector3DotProduct(n, up));
	Vector3 b = Vector3CrossProduct(n, t);

	Vector3 pt2 = center + n * height;
	Vector3 tip = reversed ? center : pt2;
	Vector3 base = reversed ? pt2 : center;

	if (!silhouette) {
		for (u32 i = 0; i < segments; i++) {
			f32 angle = f32(i) / segments * 2.0f * PI;
			Vector3 point = base +
				(t * ::cosf(angle) + b * ::sinf(angle)) * radius;
			Line(tip, point, color);
		}
	}
	else {
		Vector3 viewDir = Vector3Normalize(mViewPosition - tip);
		Vector3 axis = Vector3Normalize(base - tip);
		Vector3 right = Vector3Normalize(Vector3CrossProduct(axis, viewDir));
		Vector3 up = Vector3CrossProduct(right, axis);

		Vector3 pt1 = base + right * radius;
		Vector3 pt2 = base - right * radius;
		Line(tip, pt1, color);
		Line(tip, pt2, color);
	}

	Circle(base, n, up, radius, color, segments);
}

void DebugDraw::FillCone(
	const Vector3& center,
	const Vector3& direction,
	float radius, float height,
	const Vector4& color,
	bool reversed,
	bool drawBase
)
{
	// Using triangles
	const u32 segments = std::max(minSegmentCount, u32(std::floor(12 * std::sqrt(radius))));
	const Vector3 up = ::abs(direction.y) < 0.999f
		? Vector3{ 0.0f, 1.0f, 0.0f }
		: Vector3{ 1.0f, 0.0f, 0.0f };
	Vector3 n = Vector3Normalize(direction);
	Vector3 t = Vector3Normalize(up - n * Vector3DotProduct(n, up));
	Vector3 b = Vector3CrossProduct(n, t);

	Vector3 pt2 = center + n * height;
	Vector3 tip = reversed ? center : pt2;
	Vector3 base = reversed ? pt2 : center;

	DebugDrawCommand cmd{};
	cmd.primitiveType = DebugDrawPrimitiveType::triangle;

	auto fnGetCirclePoint = [&](u32 i) {
		f32 angle = (f32(i) / segments) * 2.0f * PI;
		return base + (t * ::cosf(angle) + b * ::sinf(angle)) * radius;
	};

	auto fnAddTriangle = [&](u32 a, u32 b, Vector3 pos, bool flipWinding) {
		Vector3 start = fnGetCirclePoint(a);
		Vector3 end = fnGetCirclePoint(b);
		cmd.vertices.emplace_back(pos, color);

		// reverse flipWinding if the cone is reversed
		flipWinding = (flipWinding && !reversed) || (!flipWinding && reversed);

		if (flipWinding) {
			std::swap(start, end);
		}

		cmd.vertices.emplace_back(start, color);
		cmd.vertices.emplace_back(end, color);
	};

	DebugDrawVertex tipVertex(tip, color);

	for (u32 i = 0; i < segments - 1; i++) {
		fnAddTriangle(i, i + 1, tip, false);
	}

	// close the cone
	fnAddTriangle(segments - 1, 0, tip, false);

	if (drawBase) {
		// base
		for (u32 i = 0; i < segments - 1; i++) {
			fnAddTriangle(i, i + 1, base, true);
		}

		// close the base
		fnAddTriangle(segments - 1, 0, base, true);
	}

	PushCommand(cmd);
}

void DebugDraw::Arrow(const Vector3& start, const Vector3& end, const Vector4& color, float arrowSize)
{
	Vector3 direction = Vector3Normalize(end - start);
	Line(start, end, color);
	FillCone(end, direction, arrowSize, arrowSize * 2.0f, color, false, false);
}

void DebugDraw::Triad(const Matrix& modelMatrix, float size, float arrowSize)
{
	Vector3 forward = Vector3Transform(Vector3{ 0.0f, 0.0f, -size }, modelMatrix);
	Vector3 right = Vector3Transform(Vector3{ size, 0.0f, 0.0f }, modelMatrix);
	Vector3 up = Vector3Transform(Vector3{ 0.0f, size, 0.0f }, modelMatrix);
	Vector3 center = Vector3Transform(Vector3{ 0.0f, 0.0f, 0.0f }, modelMatrix);

	Point(center, colors::Yellow, 8);
	Arrow(center, right, colors::Red, arrowSize);
	Arrow(center, up, colors::Green, arrowSize);
	Arrow(center, forward, colors::Blue, arrowSize);
}

void DebugDraw::Grid(
	const Vector3& center,
	const Vector2& halfSize,
	float step,
	const Vector4& color,
	const Vector3& normal
)
{
	const Vector3 up = ::abs(normal.y) < 0.999f
		? Vector3{ 0.0f, 1.0f, 0.0f }
		: Vector3{ 1.0f, 0.0f, 0.0f };
	Vector3 n = Vector3Normalize(normal);
	Vector3 t = Vector3Normalize(up - n * Vector3DotProduct(n, up));
	Vector3 b = Vector3CrossProduct(n, t);
	
	// draw horizontal lines
	for (f32 x = -halfSize.x; x <= halfSize.x; x += step) {
		Vector3 start = center + t * x - b * halfSize.y;
		Vector3 end = center + t * x + b * halfSize.y;
		Line(start, end, color);
	}

	// draw vertical lines
	for (f32 y = -halfSize.y; y <= halfSize.y; y += step) {
		Vector3 start = center + b * y - t * halfSize.x;
		Vector3 end = center + b * y + t * halfSize.x;
		Line(start, end, color);
	}
}

i32 DebugDraw::Char(i32 x, i32 y, char character, const Vector4& color, f32 scale)
{
	const Vector3 quadVerts[] = {
		{ 0.0f, 0.0f, 0.0f },
		{ 1.0f, 0.0f, 0.0f },
		{ 1.0f, 1.0f, 0.0f },
		{ 1.0f, 1.0f, 0.0f },
		{ 0.0f, 1.0f, 0.0f },
		{ 0.0f, 0.0f, 0.0f }
	};

	DebugDrawCommand cmd{};
	cmd.primitiveType = DebugDrawPrimitiveType::triangle;
	cmd.texture = mFontTexture.get();
	cmd.twoD = true;
	cmd.depthEnabled = false;
	cmd.isChar = true;

	auto fnDrawQuad = [&](i32 cx, i32 cy, f32 uvx, f32 uvy, f32 uvw, f32 uvh, f32 w, f32 h, const Vector4& color) {
		Vector3 size{ w, h, 0.0f };
		Vector3 offset{ cx, cy, 0.0f };
		for (const Vector3& v : quadVerts) {
			cmd.vertices.emplace_back(
				v * size + offset,
				Vector2{ v.x, v.y } *Vector2{ uvw, uvh } + Vector2{ uvx, uvy },
				color
			);
		}
	};

	// texture is a 256x256 font atlas of 16x16 characters of ascii (0-255)
	character &= 0xFF;

	const f32 w = 1.0f / 16.0f;
	const f32 h = 1.0f / 16.0f;

	const f32 u = (character % 16) * w;
	const f32 v = (character / 16) * h;

	const i32 size = i32(16.0f * scale);
	const i32 advance = i32(mFontCharWidths[character] * scale);

	fnDrawQuad(x+1, y+1, u, v, w, h, size, size, colors::Black);
	fnDrawQuad(x, y, u, v, w, h, size, size, color);

	PushCommand(cmd, false);

	return advance;
}

i32 DebugDraw::String(i32 x, i32 y, const str& string, f32 scale)
{
	Vector4 color = colors::White;

	i32 tx = x;
	for (size_t i = 0; i < string.size(); i++) {
		char c = string[i];
		bool isNewLine = c == '\n' || c == '\r';

		if (::isspace(c) && !isNewLine) {
			tx += i32(8.0f * scale);
			continue;
		}
		else if (isNewLine) {
			tx = x;
			y += i32(17.0f * scale);
			continue;
		}
		// parse color hints
		else if (c == '[' && string[i+1] == '/') {
			i += 2;

			str name = "";
			while (i < string.size() && string[i] != ']' && string[i] != ':') {
				name += string[i];
				i++;
			}

			int power = 100;
			if (string[i] == ':') {
				i++;
				str powerStr = "";
				while (i < string.size() && string[i] != ']') {
					if (!::isdigit(string[i])) {
						utils::LogW("Invalid power value in color hint");
						break;
					}
					powerStr += string[i];
					i++;
				}
				if (powerStr.size() > 0) {
					power = std::stoi(powerStr);
				}
			}

			std::transform(name.begin(), name.end(), name.begin(), ::tolower);
			name.erase(std::remove_if(name.begin(), name.end(), ::isspace), name.end());

			if (name == "reset" || name == "0") {
				color = colors::White;
				continue;
			}

			f32 powerFactor = f32(power) / 100.0f;
			color = GetColor(name) * powerFactor;
			color.w = 1.0f;

			if (string[i] != ']') {
				utils::LogW("Missing closing bracket for color hint");
			}
			continue;
		}
		tx += Char(tx, y, c, color, scale);
	}
	return tx;
}

Vector4 DebugDraw::GetColor(const str& colorName)
{
#pragma region Colors
	static const std::unordered_map<std::string, Vector4> colorMap = {
		{ "aliceblue", colors::AliceBlue },
		{ "antiquewhite", colors::AntiqueWhite },
		{ "aquamarine", colors::Aquamarine },
		{ "azure", colors::Azure },
		{ "beige", colors::Beige },
		{ "bisque", colors::Bisque },
		{ "black", colors::Black },
		{ "blanchedalmond", colors::BlanchedAlmond },
		{ "blue", colors::Blue },
		{ "blueviolet", colors::BlueViolet },
		{ "brown", colors::Brown },
		{ "burlywood", colors::BurlyWood },
		{ "cadetblue", colors::CadetBlue },
		{ "chartreuse", colors::Chartreuse },
		{ "chocolate", colors::Chocolate },
		{ "coral", colors::Coral },
		{ "cornflowerblue", colors::CornflowerBlue },
		{ "cornsilk", colors::Cornsilk },
		{ "crimson", colors::Crimson },
		{ "cyan", colors::Cyan },
		{ "darkblue", colors::DarkBlue },
		{ "darkcyan", colors::DarkCyan },
		{ "darkgray", colors::DarkGray },
		{ "darkgreen", colors::DarkGreen },
		{ "darkkhaki", colors::DarkKhaki },
		{ "darkmagenta", colors::DarkMagenta },
		{ "darkolivegreen", colors::DarkOliveGreen },
		{ "darkorange", colors::DarkOrange },
		{ "darkorchid", colors::DarkOrchid },
		{ "darkred", colors::DarkRed },
		{ "darksalmon", colors::DarkSalmon },
		{ "darkseagreen", colors::DarkSeaGreen },
		{ "darkslateblue", colors::DarkSlateBlue },
		{ "darkslategray", colors::DarkSlateGray },
		{ "darkturquoise", colors::DarkTurquoise },
		{ "darkviolet", colors::DarkViolet },
		{ "deeppink", colors::DeepPink },
		{ "deepskyblue", colors::DeepSkyBlue },
		{ "dimgray", colors::DimGray },
		{ "dodgerblue", colors::DodgerBlue },
		{ "firebrick", colors::FireBrick },
		{ "floralwhite", colors::FloralWhite },
		{ "forestgreen", colors::ForestGreen },
		{ "gainsboro", colors::Gainsboro },
		{ "ghostwhite", colors::GhostWhite },
		{ "gold", colors::Gold },
		{ "gray", colors::Gray },
		{ "green", colors::Green },
		{ "greenyellow", colors::GreenYellow },
		{ "hotpink", colors::HotPink },
		{ "indianred", colors::IndianRed },
		{ "indigo", colors::Indigo },
		{ "ivory", colors::Ivory },
		{ "khaki", colors::Khaki },
		{ "lavender", colors::Lavender },
		{ "lavenderblush", colors::LavenderBlush },
		{ "lawngreen", colors::LawnGreen },
		{ "lemonchiffon", colors::LemonChiffon },
		{ "lightblue", colors::LightBlue },
		{ "lightcoral", colors::LightCoral },
		{ "lightcyan", colors::LightCyan },
		{ "lightgray", colors::LightGray },
		{ "lightgreen", colors::LightGreen },
		{ "lightpink", colors::LightPink },
		{ "lightsalmon", colors::LightSalmon },
		{ "lightseagreen", colors::LightSeaGreen },
		{ "lightskyblue", colors::LightSkyBlue },
		{ "lightslategray", colors::LightSlateGray },
		{ "lightsteelblue", colors::LightSteelBlue },
		{ "lightyellow", colors::LightYellow },
		{ "lime", colors::Lime },
		{ "limegreen", colors::LimeGreen },
		{ "linen", colors::Linen },
		{ "magenta", colors::Magenta },
		{ "maroon", colors::Maroon },
		{ "mediumblue", colors::MediumBlue },
		{ "mediumorchid", colors::MediumOrchid },
		{ "mediumpurple", colors::MediumPurple },
		{ "mediumseagreen", colors::MediumSeaGreen },
		{ "mediumslateblue", colors::MediumSlateBlue },
		{ "mediumspringgreen", colors::MediumSpringGreen },
		{ "mediumturquoise", colors::MediumTurquoise },
		{ "mediumvioletred", colors::MediumVioletRed },
		{ "midnightblue", colors::MidnightBlue },
		{ "mintcream", colors::MintCream },
		{ "mistyrose", colors::MistyRose },
		{ "moccasin", colors::Moccasin },
		{ "navajowhite", colors::NavajoWhite },
		{ "navy", colors::Navy },
		{ "oldlace", colors::OldLace },
		{ "olive", colors::Olive },
		{ "olivedrab", colors::OliveDrab },
		{ "orange", colors::Orange },
		{ "orangered", colors::OrangeRed },
		{ "orchid", colors::Orchid },
		{ "palegreen", colors::PaleGreen },
		{ "paleturquoise", colors::PaleTurquoise },
		{ "palevioletred", colors::PaleVioletRed },
		{ "papayawhip", colors::PapayaWhip },
		{ "peachpuff", colors::PeachPuff },
		{ "peru", colors::Peru },
		{ "pink", colors::Pink },
		{ "plum", colors::Plum },
		{ "powderblue", colors::PowderBlue },
		{ "purple", colors::Purple },
		{ "rebeccapurple", colors::RebeccaPurple },
		{ "red", colors::Red },
		{ "rosybrown", colors::RosyBrown },
		{ "royalblue", colors::RoyalBlue },
		{ "saddlebrown", colors::SaddleBrown },
		{ "salmon", colors::Salmon },
		{ "sandybrown", colors::SandyBrown },
		{ "seagreen", colors::SeaGreen },
		{ "sienna", colors::Sienna },
		{ "silver", colors::Silver },
		{ "skyblue", colors::SkyBlue },
		{ "slateblue", colors::SlateBlue },
		{ "slategray", colors::SlateGray },
		{ "snow", colors::Snow },
		{ "springgreen", colors::SpringGreen },
		{ "steelblue", colors::SteelBlue },
		{ "tan", colors::Tan },
		{ "teal", colors::Teal },
		{ "thistle", colors::Thistle },
		{ "tomato", colors::Tomato },
		{ "turquoise", colors::Turquoise },
		{ "violet", colors::Violet },
		{ "wheat", colors::Wheat },
		{ "white", colors::White },
		{ "whitesmoke", colors::WhiteSmoke },
		{ "yellow", colors::Yellow },
		{ "yellowgreen", colors::YellowGreen }
	};

	str name = colorName;
	std::transform(name.begin(), name.end(), name.begin(), ::tolower);

	auto it = colorMap.find(name);
	if (it != colorMap.end()) return it->second;

	return colors::White;
#pragma endregion
}

void DebugDraw::Line(const Vector3& start, const Vector3& end, const Vector4& startColor, const Vector4& endColor)
{
	DebugDrawCommand cmd{};
	cmd.primitiveType = DebugDrawPrimitiveType::line;
	cmd.vertices.emplace_back(start, startColor);
	cmd.vertices.emplace_back(end, endColor);
	PushCommand(cmd);
}

void DebugDraw::BuildBatches()
{
	if (mCommands.empty()) {
		return;
	}

	std::vector<DebugDrawVertex> vertices;

	// sort by texture, primitive type and depthEnabled
	std::sort(mCommands.begin(), mCommands.end(), [](const DebugDrawCommand& a, const DebugDrawCommand& b) {
		if (a.texture != b.texture) return a.texture < b.texture;
		if (a.primitiveType != b.primitiveType) return a.primitiveType < b.primitiveType;
		if (a.depthEnabled != b.depthEnabled) return a.depthEnabled < b.depthEnabled;
		if (a.stippleFactor != b.stippleFactor) return a.stippleFactor < b.stippleFactor;
		if (a.stipplePattern != b.stipplePattern) return a.stipplePattern < b.stipplePattern;
		return a.twoD < b.twoD;
	});

	auto firstCmd = mCommands[0];

	vertices.insert(vertices.end(), firstCmd.vertices.begin(), firstCmd.vertices.end());
	mBatches.emplace_back(DebugDrawBatch{
		.primitiveType = firstCmd.primitiveType,
		.offset = 0,
		.count = static_cast<u32>(firstCmd.vertices.size()),
		.texture = firstCmd.texture,
		.depthEnabled = firstCmd.depthEnabled,
		.twoD = firstCmd.twoD,
		.isChar = firstCmd.isChar,
		.smoothPoints = firstCmd.smoothPoints,
		.stippleFactor = firstCmd.stippleFactor,
		.stipplePattern = firstCmd.stipplePattern
	});

	auto fnCmp = [](const DebugDrawCommand& a, const DebugDrawCommand& b) {
		if (a.primitiveType != b.primitiveType) {
			return true;
		}
		if (a.depthEnabled != b.depthEnabled) {
			return true;
		}
		if (a.twoD != b.twoD) {
			return true;
		}
		
		if (a.texture != b.texture) {
			return true;
		}

		if (a.primitiveType == DebugDrawPrimitiveType::line &&
			b.primitiveType == DebugDrawPrimitiveType::line) {
			if (a.stippleFactor != b.stippleFactor) {
				return true;
			}
			if (a.stipplePattern != b.stipplePattern) {
				return true;
			}
		}

		return false;
	};

	u32 offset = 0;
	for (size_t i = 1; i < mCommands.size(); i++) {
		auto& cmd = mCommands[i];
		auto& prevCmd = mCommands[i - 1];

		if (fnCmp(cmd, prevCmd)) {
			offset += mBatches.back().count;
			mBatches.emplace_back(DebugDrawBatch{
				.primitiveType = cmd.primitiveType,
				.offset = offset,
				.count = static_cast<u32>(cmd.vertices.size()),
				.texture = cmd.texture,
				.depthEnabled = cmd.depthEnabled,
				.twoD = cmd.twoD,
				.isChar = cmd.isChar,
				.smoothPoints = cmd.smoothPoints,
				.stippleFactor = cmd.stippleFactor,
				.stipplePattern = cmd.stipplePattern
			});
		}
		else {
			mBatches.back().count += static_cast<u32>(cmd.vertices.size());
		}

		// copy vertices
		vertices.insert(vertices.end(), cmd.vertices.begin(), cmd.vertices.end());
	}

	mVBO.Update(vertices.data(), vertices.size());
}

void DebugDraw::PushCommand(DebugDrawCommand cmd, bool overrideState)
{
	if (cmd.vertices.empty()) {
		return;
	}

	if (overrideState) {
		cmd.depthEnabled = mDepthEnabled;
		cmd.twoD = mTwoDMode;
	}
	cmd.stipplePattern = mStipplePattern;
	cmd.stippleFactor = mStippleFactor;
	mCommands.push_back(cmd);
}
