#pragma once

#include "../rendering/rendering_engine.h"
#include "../utils/custom_types.h"
#include "component.h"

#include <unordered_map>
#include <typeindex>
#include <concepts>

class RENGINE_API Entity {
public:
	Entity() = default;
	~Entity() = default;

	// Make Entity non-copyable
	Entity(const Entity&) = delete;
	Entity& operator=(const Entity&) = delete;

	// Entity can still be moved (default move constructor and assignment are fine)
	Entity(Entity&&) = default;
	Entity& operator=(Entity&&) = default;

	template <std::derived_from<Component> T, typename... Args>
	T* AddComponent(Args&&... args) {
		auto component = std::make_unique<T>(std::forward<Args>(args)...);
		component->mOwner = this;

		auto [it, success] = mComponents.emplace(typeid(T), std::move(component));
		return static_cast<T*>(mComponents[typeid(T)].get());
	}

	template <std::derived_from<Component> T>
	T* GetComponent() {
		auto it = mComponents.find(typeid(T));
		if (it != mComponents.end()) {
			return static_cast<T*>(it->second.get());
		}
		return nullptr;
	}

	template <std::derived_from<Component> T>
	void RemoveComponent() {
		mComponents.erase(typeid(T));
	}

	template <std::derived_from<Component> T>
	bool HasComponent() {
		return mComponents.contains(typeid(T));
	}

	void Update(InputSystem& input, f32 deltaTime);
	void Render(RenderingEngine& renderer);

	void SetLifeTime(f32 lifeTime) { mLifeTime = lifeTime; }
	f32 GetLifeTime() const { return mLifeTime; }

	bool IsDead() const { return mDead; }

	str GetTag() const { return mTag; }
	void SetTag(const str& tag) { mTag = tag; }

private:
	std::unordered_map<std::type_index, UPtr<Component>> mComponents;
	float mLifeTime{ -1.0f };
	bool mDead{ false };
	str mTag;
};
