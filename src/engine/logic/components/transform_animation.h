#pragma once

#include "../../external/raymath/raymath.h"
#include "../component.h"

#include <vector>

struct RENGINE_API TransformAnmiationKeyframe {
	u32 frame;
	Vector3 position;
	Quaternion rotation;
	Vector3 scale;
};

class RENGINE_API TransformAnimation : public Component {
public:
	TransformAnimation() = default;

	void OnUpdate(InputSystem& input, f32 deltaTime) override;

	void AddKeyframe(u32 frame, const Vector3& position, const Quaternion& rotation, const Vector3& scale);
	void ClearKeyframes();

	void SetKeyframe(u32 frame, const Vector3& position, const Quaternion& rotation, const Vector3& scale);

	f32 GetTimeScale() const { return mTimeScale; }
	void SetTimeScale(f32 timeScale) { mTimeScale = timeScale; }

	bool IsLoop() const { return mLoop; }
	void SetLoop(bool loop) { mLoop = loop; }

	u32 GetFrameRate() const { return mFrameRate; }
	void SetFrameRate(u32 frameRate) { mFrameRate = frameRate; }

	u32 GetCurrentFrame() const { return mCurrentFrame; }
	void SetCurrentFrame(u32 frame) { mCurrentFrame = frame; }

	void Play();
	void Stop();

private:
	f32 mTimeScale{ 1.0f };
	f32 mFrameTime{ 0.0f };

	bool mLoop{ true };
	bool mIsPlaying{ false };

	u32 mFrameRate{ 30 };
	u32 mCurrentFrame{ 0 };
	u32 mStartFrame{ 99999 };
	u32 mEndFrame{ 0 };

	std::vector<TransformAnmiationKeyframe> mKeyframes;

	void Interpolate(u32 frame);
	void Sort();
};