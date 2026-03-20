#pragma once

#include "entity.h"
#include <vector>

#include "../utils/pool.h"

class Scene {
public:
	Scene();
	~Scene() = default;

	Entity* Create();
	void Destroy(Entity* entity);

	Entity* FindByTag(const str& tag);

	void Update(InputSystem& input, f32 deltaTime);
	void Render(RenderingEngine& renderer);

	std::vector<Entity*> GetEntities() {
		return mEntityPool.GetActive();
	}

private:
	Pool<Entity> mEntityPool;
	std::vector<Entity*> mDeadEntities;
};