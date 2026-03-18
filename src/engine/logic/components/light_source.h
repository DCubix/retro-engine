#pragma once

#include "../component.h"
#include "../../rendering/light.h"

class RENGINE_API LightSource : public Component {
public:
	LightSource() = default;
	~LightSource() = default;

	void OnRender(RenderingEngine& renderer) override;

	LightType GetType() const { return mLight.type; }
	void SetType(LightType type) { mLight.type = type; }

	Vector3 GetColor() const { return mLight.color; }
	void SetColor(const Vector3& color) { mLight.color = color; }

	f32 GetIntensity() const { return mLight.intensity; }
	void SetIntensity(f32 intensity) { mLight.intensity = intensity; }

	f32 GetRadius() const { return mLight.radius; }
	void SetRadius(f32 radius) { mLight.radius = radius; }

	f32 GetCutoffAngle() const { return mLight.cutoffAngle; }
	void SetCutoffAngle(f32 angle) { mLight.cutoffAngle = angle; }

	bool IsShadowCaster() const { return mLight.shadowsCaster; }
	void SetShadowCaster(bool caster) { mLight.shadowsCaster = caster; }

private:
	Light mLight{};
};