#pragma once

#include "buffer.h"
#include "texture.h"
#include "shader.h"

#include "../external/raymath/raymath.h"

struct DebugDrawVertex {
	Vector3 position;
    Vector2 uv{ 0.0f, 0.0f };
	Vector4 color;
    float pointSize{ 1.0f };

	DebugDrawVertex() = default;
	DebugDrawVertex(const Vector3& pos)
		: position(pos) {}
	DebugDrawVertex(const Vector3& pos, const Vector2& uv)
		: position(pos), uv(uv) {}
	DebugDrawVertex(const Vector3& pos, float size)
		: position(pos), pointSize(size) {}
	DebugDrawVertex(const Vector3& pos, const Vector4& col)
		: position(pos), color(col) {}
    DebugDrawVertex(const Vector3& pos, const Vector4& col, float size)
        : position(pos), color(col), pointSize(size) {}
	DebugDrawVertex(const Vector3& pos, const Vector2& uv, const Vector4& col)
		: position(pos), uv(uv), color(col) {}
	DebugDrawVertex(const Vector3& pos, const Vector2& uv, const Vector4& col, float size)
		: position(pos), uv(uv), color(col), pointSize(size) {}
};

enum class DebugDrawPrimitiveType {
	point,
	line,
	triangle,
	triangleFan,
};

struct DebugDrawCommand {
	DebugDrawPrimitiveType primitiveType;
	std::vector<DebugDrawVertex> vertices;
	Texture* texture{ nullptr };
	bool depthEnabled{ false };
	bool twoD{ false };
	bool isChar{ false };
	bool smoothPoints{ false };
	float stippleFactor{ 1.0f };
	u32 stipplePattern{ 0x0 };
};

struct DebugDrawBatch {
	DebugDrawPrimitiveType primitiveType;
	u32 offset; // offset in the vertex buffer
	u32 count; // number of vertices
	Texture* texture{ nullptr };
	bool depthEnabled{ false };
	bool twoD{ false };
	bool isChar{ false };
	bool smoothPoints{ false };
    float stippleFactor{ 1.0f };
    u32 stipplePattern{ 0x0 };
};

class DebugDraw {
public:
	DebugDraw() = default;
	DebugDraw(u32 windowWidth, u32 windowHeight);

	void Begin(const Vector3& viewPosition);
	void End(const Matrix& mvp);

	void SetDepthEnabled(bool enabled) { mDepthEnabled = enabled; }
	void SetTwoDMode(bool enabled) { mTwoDMode = enabled; }
	void SetStipplePattern(u32 pattern) { mStipplePattern = pattern; }
	void SetStippleFactor(float factor) { mStippleFactor = factor; }

	// draw functions
	void Point(const Vector3& position, const Vector4& color, float size = 1.0f, bool smooth = false);
	void Line(const Vector3& start, const Vector3& end, const Vector4& startColor, const Vector4& endColor);
	void Line(const Vector3& start, const Vector3& end, const Vector4& color);
	void LineLoop(const std::vector<Vector3>& points, const std::vector<Vector4>& colors);
	void LineLoop(const std::vector<Vector3>& points, const Vector4& color);

	void Circle(
        const Vector3& center,
        const Vector3& normal,
        const Vector3& up,
        float radius,
        const Vector4& color,
		u32 segmentsOverride = 0
    );
	void FillCircle(
		const Vector3& center,
		const Vector3& normal,
		const Vector3& up,
		float radius,
		const Vector4& color
	);

	void Cube(
		const Vector3& center,
		const Vector3& size,
		const Vector4& color,
		const Matrix& transform = MatrixIdentity()
	);
	void FillCube(
		const Vector3& center,
		const Vector3& size,
		const Vector4& color,
		const Matrix& transform = MatrixIdentity()
	);

	void Sphere(
		const Vector3& center,
		float radius,
		const Vector4& color
	);
	void FillSphere(
		const Vector3& center,
		float radius,
		const Vector4& color
	);

	void Cone(
		const Vector3& center,
		const Vector3& direction,
		float radius,
		float height,
		const Vector4& color,
		bool reversed = false,
		bool silhouette = false
	);
	void FillCone(
		const Vector3& center,
		const Vector3& direction,
		float radius,
		float height,
		const Vector4& color,
        bool reversed = false,
		bool drawBase = true
	);

	void Arrow(
		const Vector3& start,
		const Vector3& end,
		const Vector4& color,
        float arrowSize = 0.2f
	);

    void Triad(
        const Matrix& modelMatrix,
        float size = 1.0f,
        float arrowSize = 0.2f
    );

    void Grid(
        const Vector3& center,
        const Vector2& halfSize,
        float step,
        const Vector4& color,
        const Vector3& normal = { 0.0f, 1.0f, 0.0f }
    );

	i32 Char(
		i32 x, i32 y,
		char character,
		const Vector4& color,
		f32 scale = 1.0f
	);
	i32 String(
		i32 x, i32 y,
		const str& string,
		f32 scale = 1.0f
	);

	static Vector4 GetColor(const str& colorName);

private:
	UPtr<Shader> mShader;

	VertexArray mVAO;
	VertexBuffer<DebugDrawVertex> mVBO;

	UPtr<Texture> mFontTexture;
    std::map<char, u32> mFontCharWidths;

	std::vector<DebugDrawCommand> mCommands;
	std::vector<DebugDrawBatch> mBatches;

	u32 mWindowWidth{ 0 },
		mWindowHeight{ 0 };

	bool mDepthEnabled{ true };
	bool mTwoDMode{ false };
	u32 mStipplePattern{ 0xFFFFFFFF };
	float mStippleFactor{ 1.0f };

	bool mEnded{ true };

    Vector3 mViewPosition{};

	void BuildBatches();
	void PushCommand(DebugDrawCommand cmd, bool overrideState = true);
};

namespace colors {
    const Vector4 AliceBlue = { 0.941176f, 0.972549f, 1.000000f, 1.0f };  
    const Vector4 AntiqueWhite = { 0.980392f, 0.921569f, 0.843137f, 1.0f };  
    const Vector4 Aquamarine = { 0.498039f, 1.000000f, 0.831373f, 1.0f };  
    const Vector4 Azure = { 0.941176f, 1.000000f, 1.000000f, 1.0f };  
    const Vector4 Beige = { 0.960784f, 0.960784f, 0.862745f, 1.0f };  
    const Vector4 Bisque = { 1.000000f, 0.894118f, 0.768627f, 1.0f };  
    const Vector4 Black = { 0.000000f, 0.000000f, 0.000000f, 1.0f };  
    const Vector4 BlanchedAlmond = { 1.000000f, 0.921569f, 0.803922f, 1.0f };  
    const Vector4 Blue = { 0.000000f, 0.000000f, 1.000000f, 1.0f };  
    const Vector4 BlueViolet = { 0.541176f, 0.168627f, 0.886275f, 1.0f };  
    const Vector4 Brown = { 0.647059f, 0.164706f, 0.164706f, 1.0f };  
    const Vector4 BurlyWood = { 0.870588f, 0.721569f, 0.529412f, 1.0f };  
    const Vector4 CadetBlue = { 0.372549f, 0.619608f, 0.627451f, 1.0f };  
    const Vector4 Chartreuse = { 0.498039f, 1.000000f, 0.000000f, 1.0f };  
    const Vector4 Chocolate = { 0.823529f, 0.411765f, 0.117647f, 1.0f };  
    const Vector4 Coral = { 1.000000f, 0.498039f, 0.313726f, 1.0f };  
    const Vector4 CornflowerBlue = { 0.392157f, 0.584314f, 0.929412f, 1.0f };  
    const Vector4 Cornsilk = { 1.000000f, 0.972549f, 0.862745f, 1.0f };  
    const Vector4 Crimson = { 0.862745f, 0.078431f, 0.235294f, 1.0f };  
    const Vector4 Cyan = { 0.000000f, 1.000000f, 1.000000f, 1.0f };  
    const Vector4 DarkBlue = { 0.000000f, 0.000000f, 0.545098f, 1.0f };  
    const Vector4 DarkCyan = { 0.000000f, 0.545098f, 0.545098f, 1.0f };  
    const Vector4 DarkGoldenRod = { 0.721569f, 0.525490f, 0.043137f, 1.0f };  
    const Vector4 DarkGray = { 0.662745f, 0.662745f, 0.662745f, 1.0f };  
    const Vector4 DarkGreen = { 0.000000f, 0.392157f, 0.000000f, 1.0f };  
    const Vector4 DarkKhaki = { 0.741176f, 0.717647f, 0.419608f, 1.0f };  
    const Vector4 DarkMagenta = { 0.545098f, 0.000000f, 0.545098f, 1.0f };  
    const Vector4 DarkOliveGreen = { 0.333333f, 0.419608f, 0.184314f, 1.0f };  
    const Vector4 DarkOrange = { 1.000000f, 0.549020f, 0.000000f, 1.0f };  
    const Vector4 DarkOrchid = { 0.600000f, 0.196078f, 0.800000f, 1.0f };  
    const Vector4 DarkRed = { 0.545098f, 0.000000f, 0.000000f, 1.0f };  
    const Vector4 DarkSalmon = { 0.913725f, 0.588235f, 0.478431f, 1.0f };  
    const Vector4 DarkSeaGreen = { 0.560784f, 0.737255f, 0.560784f, 1.0f };  
    const Vector4 DarkSlateBlue = { 0.282353f, 0.239216f, 0.545098f, 1.0f };  
    const Vector4 DarkSlateGray = { 0.184314f, 0.309804f, 0.309804f, 1.0f };  
    const Vector4 DarkTurquoise = { 0.000000f, 0.807843f, 0.819608f, 1.0f };  
    const Vector4 DarkViolet = { 0.580392f, 0.000000f, 0.827451f, 1.0f };  
    const Vector4 DeepPink = { 1.000000f, 0.078431f, 0.576471f, 1.0f };  
    const Vector4 DeepSkyBlue = { 0.000000f, 0.749020f, 1.000000f, 1.0f };  
    const Vector4 DimGray = { 0.411765f, 0.411765f, 0.411765f, 1.0f };  
    const Vector4 DodgerBlue = { 0.117647f, 0.564706f, 1.000000f, 1.0f };  
    const Vector4 FireBrick = { 0.698039f, 0.133333f, 0.133333f, 1.0f };  
    const Vector4 FloralWhite = { 1.000000f, 0.980392f, 0.941176f, 1.0f };  
    const Vector4 ForestGreen = { 0.133333f, 0.545098f, 0.133333f, 1.0f };  
    const Vector4 Gainsboro = { 0.862745f, 0.862745f, 0.862745f, 1.0f };  
    const Vector4 GhostWhite = { 0.972549f, 0.972549f, 1.000000f, 1.0f };  
    const Vector4 Gold = { 1.000000f, 0.843137f, 0.000000f, 1.0f };  
    const Vector4 GoldenRod = { 0.854902f, 0.647059f, 0.125490f, 1.0f };  
    const Vector4 Gray = { 0.501961f, 0.501961f, 0.501961f, 1.0f };  
    const Vector4 Green = { 0.000000f, 0.501961f, 0.000000f, 1.0f };  
    const Vector4 GreenYellow = { 0.678431f, 1.000000f, 0.184314f, 1.0f };  
    const Vector4 HoneyDew = { 0.941176f, 1.000000f, 0.941176f, 1.0f };  
    const Vector4 HotPink = { 1.000000f, 0.411765f, 0.705882f, 1.0f };  
    const Vector4 IndianRed = { 0.803922f, 0.360784f, 0.360784f, 1.0f };  
    const Vector4 Indigo = { 0.294118f, 0.000000f, 0.509804f, 1.0f };  
    const Vector4 Ivory = { 1.000000f, 1.000000f, 0.941176f, 1.0f };  
    const Vector4 Khaki = { 0.941176f, 0.901961f, 0.549020f, 1.0f };  
    const Vector4 Lavender = { 0.901961f, 0.901961f, 0.980392f, 1.0f };  
    const Vector4 LavenderBlush = { 1.000000f, 0.941176f, 0.960784f, 1.0f };  
    const Vector4 LawnGreen = { 0.486275f, 0.988235f, 0.000000f, 1.0f };  
    const Vector4 LemonChiffon = { 1.000000f, 0.980392f, 0.803922f, 1.0f };  
    const Vector4 LightBlue = { 0.678431f, 0.847059f, 0.901961f, 1.0f };  
    const Vector4 LightCoral = { 0.941176f, 0.501961f, 0.501961f, 1.0f };  
    const Vector4 LightCyan = { 0.878431f, 1.000000f, 1.000000f, 1.0f };  
    const Vector4 LightGoldenYellow = { 0.980392f, 0.980392f, 0.823529f, 1.0f };  
    const Vector4 LightGray = { 0.827451f, 0.827451f, 0.827451f, 1.0f };  
    const Vector4 LightGreen = { 0.564706f, 0.933333f, 0.564706f, 1.0f };  
    const Vector4 LightPink = { 1.000000f, 0.713726f, 0.756863f, 1.0f };  
    const Vector4 LightSalmon = { 1.000000f, 0.627451f, 0.478431f, 1.0f };  
    const Vector4 LightSeaGreen = { 0.125490f, 0.698039f, 0.666667f, 1.0f };  
    const Vector4 LightSkyBlue = { 0.529412f, 0.807843f, 0.980392f, 1.0f };  
    const Vector4 LightSlateGray = { 0.466667f, 0.533333f, 0.600000f, 1.0f };  
    const Vector4 LightSteelBlue = { 0.690196f, 0.768627f, 0.870588f, 1.0f };  
    const Vector4 LightYellow = { 1.000000f, 1.000000f, 0.878431f, 1.0f };  
    const Vector4 Lime = { 0.000000f, 1.000000f, 0.000000f, 1.0f };  
    const Vector4 LimeGreen = { 0.196078f, 0.803922f, 0.196078f, 1.0f };  
    const Vector4 Linen = { 0.980392f, 0.941176f, 0.901961f, 1.0f };  
    const Vector4 Magenta = { 1.000000f, 0.000000f, 1.000000f, 1.0f };  
    const Vector4 Maroon = { 0.501961f, 0.000000f, 0.000000f, 1.0f };  
    const Vector4 MediumAquaMarine = { 0.400000f, 0.803922f, 0.666667f, 1.0f };  
    const Vector4 MediumBlue = { 0.000000f, 0.000000f, 0.803922f, 1.0f };  
    const Vector4 MediumOrchid = { 0.729412f, 0.333333f, 0.827451f, 1.0f };  
    const Vector4 MediumPurple = { 0.576471f, 0.439216f, 0.858824f, 1.0f };  
    const Vector4 MediumSeaGreen = { 0.235294f, 0.701961f, 0.443137f, 1.0f };  
    const Vector4 MediumSlateBlue = { 0.482353f, 0.407843f, 0.933333f, 1.0f };  
    const Vector4 MediumSpringGreen = { 0.000000f, 0.980392f, 0.603922f, 1.0f };  
    const Vector4 MediumTurquoise = { 0.282353f, 0.819608f, 0.800000f, 1.0f };  
    const Vector4 MediumVioletRed = { 0.780392f, 0.082353f, 0.521569f, 1.0f };  
    const Vector4 MidnightBlue = { 0.098039f, 0.098039f, 0.439216f, 1.0f };  
    const Vector4 MintCream = { 0.960784f, 1.000000f, 0.980392f, 1.0f };  
    const Vector4 MistyRose = { 1.000000f, 0.894118f, 0.882353f, 1.0f };  
    const Vector4 Moccasin = { 1.000000f, 0.894118f, 0.709804f, 1.0f };  
    const Vector4 NavajoWhite = { 1.000000f, 0.870588f, 0.678431f, 1.0f };  
    const Vector4 Navy = { 0.000000f, 0.000000f, 0.501961f, 1.0f };  
    const Vector4 OldLace = { 0.992157f, 0.960784f, 0.901961f, 1.0f };  
    const Vector4 Olive = { 0.501961f, 0.501961f, 0.000000f, 1.0f };  
    const Vector4 OliveDrab = { 0.419608f, 0.556863f, 0.137255f, 1.0f };  
    const Vector4 Orange = { 1.000000f, 0.647059f, 0.000000f, 1.0f };  
    const Vector4 OrangeRed = { 1.000000f, 0.270588f, 0.000000f, 1.0f };  
    const Vector4 Orchid = { 0.854902f, 0.439216f, 0.839216f, 1.0f };  
    const Vector4 PaleGoldenRod = { 0.933333f, 0.909804f, 0.666667f, 1.0f };  
    const Vector4 PaleGreen = { 0.596078f, 0.984314f, 0.596078f, 1.0f };  
    const Vector4 PaleTurquoise = { 0.686275f, 0.933333f, 0.933333f, 1.0f };  
    const Vector4 PaleVioletRed = { 0.858824f, 0.439216f, 0.576471f, 1.0f };  
    const Vector4 PapayaWhip = { 1.000000f, 0.937255f, 0.835294f, 1.0f };  
    const Vector4 PeachPuff = { 1.000000f, 0.854902f, 0.725490f, 1.0f };  
    const Vector4 Peru = { 0.803922f, 0.521569f, 0.247059f, 1.0f };  
    const Vector4 Pink = { 1.000000f, 0.752941f, 0.796078f, 1.0f };  
    const Vector4 Plum = { 0.866667f, 0.627451f, 0.866667f, 1.0f };  
    const Vector4 PowderBlue = { 0.690196f, 0.878431f, 0.901961f, 1.0f };  
    const Vector4 Purple = { 0.501961f, 0.000000f, 0.501961f, 1.0f };  
    const Vector4 RebeccaPurple = { 0.400000f, 0.200000f, 0.600000f, 1.0f };  
    const Vector4 Red = { 1.000000f, 0.000000f, 0.000000f, 1.0f };  
    const Vector4 RosyBrown = { 0.737255f, 0.560784f, 0.560784f, 1.0f };  
    const Vector4 RoyalBlue = { 0.254902f, 0.411765f, 0.882353f, 1.0f };  
    const Vector4 SaddleBrown = { 0.545098f, 0.270588f, 0.074510f, 1.0f };  
    const Vector4 Salmon = { 0.980392f, 0.501961f, 0.447059f, 1.0f };  
    const Vector4 SandyBrown = { 0.956863f, 0.643137f, 0.376471f, 1.0f };  
    const Vector4 SeaGreen = { 0.180392f, 0.545098f, 0.341176f, 1.0f };  
    const Vector4 SeaShell = { 1.000000f, 0.960784f, 0.933333f, 1.0f };  
    const Vector4 Sienna = { 0.627451f, 0.321569f, 0.176471f, 1.0f };  
    const Vector4 Silver = { 0.752941f, 0.752941f, 0.752941f, 1.0f };  
    const Vector4 SkyBlue = { 0.529412f, 0.807843f, 0.921569f, 1.0f };  
    const Vector4 SlateBlue = { 0.415686f, 0.352941f, 0.803922f, 1.0f };  
    const Vector4 SlateGray = { 0.439216f, 0.501961f, 0.564706f, 1.0f };  
    const Vector4 Snow = { 1.000000f, 0.980392f, 0.980392f, 1.0f };  
    const Vector4 SpringGreen = { 0.000000f, 1.000000f, 0.498039f, 1.0f };  
    const Vector4 SteelBlue = { 0.274510f, 0.509804f, 0.705882f, 1.0f };  
    const Vector4 Tan = { 0.823529f, 0.705882f, 0.549020f, 1.0f };  
    const Vector4 Teal = { 0.000000f, 0.501961f, 0.501961f, 1.0f };  
    const Vector4 Thistle = { 0.847059f, 0.749020f, 0.847059f, 1.0f };  
    const Vector4 Tomato = { 1.000000f, 0.388235f, 0.278431f, 1.0f };  
    const Vector4 Turquoise = { 0.250980f, 0.878431f, 0.815686f, 1.0f };  
    const Vector4 Violet = { 0.933333f, 0.509804f, 0.933333f, 1.0f };  
    const Vector4 Wheat = { 0.960784f, 0.870588f, 0.701961f, 1.0f };  
    const Vector4 White = { 1.000000f, 1.000000f, 1.000000f, 1.0f };  
    const Vector4 WhiteSmoke = { 0.960784f, 0.960784f, 0.960784f, 1.0f };  
    const Vector4 Yellow = { 1.000000f, 1.000000f, 0.000000f, 1.0f };  
    const Vector4 YellowGreen = { 0.603922f, 0.803922f, 0.196078f, 1.0f };
}