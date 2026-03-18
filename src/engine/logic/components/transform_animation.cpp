#include "transform_animation.h"

#include "../entity.h"
#include "transform.h"

#include <algorithm>

void TransformAnimation::OnUpdate(InputSystem& input, f32 deltaTime)
{
	if (!mIsPlaying) return;
	mFrameTime += deltaTime * mTimeScale;

	if (mFrameTime >= 1.0f / static_cast<f32>(mFrameRate)) {
		mFrameTime = 0.0f;

		mCurrentFrame++;
		if (mCurrentFrame > mEndFrame) {
			if (mLoop) {
				mCurrentFrame = mStartFrame;
			}
			else {
				Stop();
				return;
			}
		}

		Interpolate(mCurrentFrame);
	}
}

void TransformAnimation::AddKeyframe(u32 frame, const Vector3& position, const Quaternion& rotation, const Vector3& scale)
{
	mKeyframes.push_back({ frame, position, rotation, scale });
	mStartFrame = std::min(mStartFrame, frame);
	mEndFrame = std::max(mEndFrame, frame);
	Sort();
}

void TransformAnimation::ClearKeyframes()
{
	mKeyframes.clear();
}

void TransformAnimation::SetKeyframe(u32 frame, const Vector3& position, const Quaternion& rotation, const Vector3& scale)
{
	for (auto& keyframe : mKeyframes) {
		if (keyframe.frame == frame) {
			keyframe.position = position;
			keyframe.rotation = rotation;
			keyframe.scale = scale;
			return;
		}
	}
	AddKeyframe(frame, position, rotation, scale);
}

void TransformAnimation::Play()
{
	if (mIsPlaying) return;
	mIsPlaying = true;
	mCurrentFrame = mStartFrame;
}

void TransformAnimation::Stop()
{
	if (!mIsPlaying) return;
	mIsPlaying = false;
	mCurrentFrame = mStartFrame;
}

void TransformAnimation::Sort()
{
	std::sort(mKeyframes.begin(), mKeyframes.end(), [](const TransformAnmiationKeyframe& a, const TransformAnmiationKeyframe& b) {
		return a.frame < b.frame;
	});
}

void TransformAnimation::Interpolate(u32 frame) {
	if (mKeyframes.empty()) return;

	auto transform = mOwner->GetComponent<Transform>();
	if (!transform) return;

	// Find the two keyframes surrounding the given frame
	TransformAnmiationKeyframe* prev = nullptr;
	TransformAnmiationKeyframe* next = nullptr;
	for (auto& keyframe : mKeyframes) {
		if (keyframe.frame <= frame) {
			prev = &keyframe;
		}
		else {
			next = &keyframe;
			break;
		}
	}
	if (!prev || !next) return;

	f32 t = static_cast<f32>(frame - prev->frame) / static_cast<f32>(next->frame - prev->frame);
	
	Vector3 position = Vector3Lerp(prev->position, next->position, t);
	Quaternion rotation = QuaternionSlerp(prev->rotation, next->rotation, t);
	Vector3 scale = Vector3Lerp(prev->scale, next->scale, t);

	transform->SetPosition(position);
	transform->SetRotation(rotation);
	transform->SetScale(scale);
}
