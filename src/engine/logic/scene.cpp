#include "scene.h"

#include "components/transform.h"

#include <mutex>

Scene::Scene()
{
	mEntityPool.Reserve(1000);
}

Entity* Scene::Create()
{
	return mEntityPool.Acquire();
}

void Scene::Destroy(Entity* entity)
{
	mEntityPool.Release(entity);
}

Entity* Scene::FindByTag(const str& tag)
{
	for (auto& entity : mEntityPool.GetActive()) {
		if (entity->GetTag() == tag) {
			return entity;
		}
	}
	return nullptr;
}

void Scene::Update(InputSystem& input, f32 deltaTime)
{
	std::mutex updateMutex;
	updateMutex.lock();
	mEntityPool.ParallelForEach([&](Entity* entity) {
		entity->Update(input, deltaTime);
		if (entity->IsDead()) {
			mDeadEntities.push_back(entity);
		}
	});
	updateMutex.unlock();

	// Remove dead entities
	for (auto it = mDeadEntities.begin(); it != mDeadEntities.end(); ++it) {
		Entity* entity = *it;
		mEntityPool.Release(entity);
		it = mDeadEntities.erase(it);
		if (it == mDeadEntities.end()) break;
	}
}

void Scene::Render(RenderingEngine& renderer)
{
	mEntityPool.ForEach([&](Entity* entity) {
		entity->Render(renderer);
	});
}
