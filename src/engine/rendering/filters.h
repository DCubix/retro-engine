#pragma once

#include "texture.h"
#include "shader.h"
#include "../external/raymath/raymath.h"

#include <vector>

using TextureChannels = std::vector<Texture*>;

enum class FilterInput {
	lightingPass = 0,
	diffuseRoughness,
	emissiveMetallic,
	position,
	normal
};

class Filter : public Shader {
public:
	Filter(const str& src);

	virtual u32 GetPassCount() const { return 1; }

	/// <summary>
	/// Run after every pass
	/// </summary>
	virtual void OnSetup() = 0;

	virtual void Reset() {}

	FilterInput GetInput() const { return mInput; }
	void SetInput(FilterInput input) { mInput = input; }

protected:
	FilterInput mInput{ FilterInput::lightingPass };
};

class BlurFilter : public Filter {
public:
	BlurFilter();

	void OnSetup() override;
	void Reset() override;

	u32 GetPassCount() const override { return mPassCount; }
	void SetPassCount(u32 count) { mPassCount = count; }

	float GetLod() const { return mLod; }
	void SetLod(float lod) { mLod = lod; }

private:
	u32 mPassCount{ 2 };
	bool mHorizontal{ false };
	float mLod{ 0.0f };
};

enum class CombineMode {
	add = 0,
	multiply,
};

class CombineFilter : public Filter {
public:
	CombineFilter();

	void OnSetup() override;

	u32 GetPassCount() const override { return 1; }

	CombineMode GetCombineMode() const { return mCombineMode; }
	void SetCombineMode(CombineMode mode) { mCombineMode = mode; }

private:
	CombineMode mCombineMode{ CombineMode::add };
};