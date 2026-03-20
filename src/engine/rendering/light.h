#pragma once

#include "../external/raymath/raymath.h"
#include "../utils/custom_types.h"

enum class LightType {
	directional = 0,
	point,
	spot
};

struct Light {
	LightType type;

	Vector3 color;
	f32 intensity;

	Vector3 position;
	Vector3 direction;
	f32 radius;

	f32 cutoffAngle;

	bool shadowsCaster{ false };

	Matrix GetViewProjection(u32 face = 0) const;
};

struct alignas(16) LightShaderData {
	alignas(16) float colorIntensity[4];
	alignas(16) float positionRadius[4];
	alignas(16) float directionCutoffAngle[4];
	int lightType;
	float _padding0;
	float _padding1;
	float _padding2;
};