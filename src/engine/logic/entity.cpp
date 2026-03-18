#include "entity.h"

void Entity::Update(InputSystem& input, f32 deltaTime)
{
	if (mDead) return;

	bool isImmortal = mLifeTime < 0.0f;

	if (!isImmortal) {
		mLifeTime -= deltaTime;
		if (mLifeTime <= 0.0f) {
			mDead = true;
		}
	}

	for (auto& [typeId, component] : mComponents) {
		if (!component->mIsActive) continue;

		if (!component->mIsCreated) {
			component->OnCreate();
			component->mIsCreated = true;
		}
		component->OnUpdate(input, deltaTime);
	}

	for (auto& [typeId, component] : mComponents) {
		if (!component->mIsActive) continue;
		if (mDead) {
			component->OnDestroy();
		}
	}
}

void Entity::Render(RenderingEngine& renderer)
{
	for (auto& [typeId, component] : mComponents) {
		if (!component->mIsActive) continue;
		component->OnRender(renderer);
	}
}
