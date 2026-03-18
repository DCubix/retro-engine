#pragma once

#include "../utils/custom_types.h"
#include "../rendering/rendering_engine.h"
#include "../core/input_system.h"

class Entity;

class RENGINE_API Component {
public:
	virtual ~Component() = default;

	virtual void OnCreate() {}
	virtual void OnDestroy() {}

	virtual void OnUpdate(InputSystem& input, f32 deltaTime) {}
	virtual void OnRender(RenderingEngine& renderer) {}
	
	Entity* GetOwner() const { return mOwner; }

	bool IsActive() const { return mIsActive; }
	void SetActive(bool active) { mIsActive = active; }

protected:
	friend class Entity;
	Entity* mOwner;
	bool mIsActive{ true };
	bool mIsCreated{ false };
};