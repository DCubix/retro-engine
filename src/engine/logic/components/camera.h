#pragma once

#include "../component.h"

class RENGINE_API Camera : public Component {
public:
	Camera() = default;
	Camera(f32 fov, f32 zNear, f32 zFar)
		: mFoV(fov), mZNear(zNear), mZFar(zFar) {}

	void OnRender(RenderingEngine& renderer) override;

	f32 GetFoV() const { return mFoV; }
	void SetFoV(f32 fov) { mFoV = fov; }

	f32 GetZNear() const { return mZNear; }
	void SetZNear(f32 zNear) { mZNear = zNear; }

	f32 GetZFar() const { return mZFar; }
	void SetZFar(f32 zFar) { mZFar = zFar; }
private:
	f32 mFoV{ 60.0f };
	f32 mZNear{ 0.1f };
	f32 mZFar{ 500.0f };
};