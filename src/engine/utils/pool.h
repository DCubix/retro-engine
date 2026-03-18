#pragma once

#include "custom_types.h"
#include <vector>

#include <execution>
#include <algorithm>
#include <functional>
#include <unordered_set>

template <typename T>
class Pool {
public:
	Pool() = default;
	~Pool() = default;

	Pool(const Pool&) = delete;
	Pool& operator=(const Pool&) = delete;

	Pool(Pool&&) = default;
	Pool& operator=(Pool&&) = default;

	void Reserve(size_t size) {
		mPool.reserve(size);
		for (size_t i = 0; i < size; ++i) {
			mPool.push_back(std::make_unique<T>());
			mFreeIndices.push_back(i);
		}
	}

	T* Acquire() {
		if (mFreeIndices.empty()) {
			mPool.push_back(std::make_unique<T>());
			mFreeIndices.push_back(mPool.size() - 1);
			return Acquire();
		}
		size_t index = mFreeIndices.back();
		mFreeIndices.pop_back();
		mActiveRefs.push_back(mPool[index].get());
		return mActiveRefs.back();
	}

	void Release(T* obj) {
		auto it = std::find_if(
			mPool.begin(), mPool.end(),
			[obj](const UPtr<T>& ptr) { return ptr.get() == obj; }
		);
		if (it != mPool.end()) {
			// Remove the object from active references
			mActiveRefs.erase(std::remove(mActiveRefs.begin(), mActiveRefs.end(), obj), mActiveRefs.end());

			size_t index = std::distance(mPool.begin(), it);
			mFreeIndices.push_back(index);
		}
	}

	template <typename Func>
	void ForEach(Func&& func) {
		for (auto obj : GetActive()) {
			func(obj);
		}
	}

	template <typename Func>
	void ParallelForEach(Func&& func) {
		std::for_each(std::execution::par, mActiveRefs.begin(), mActiveRefs.end(), func);
	}

	std::vector<T*> GetActive() const { return mActiveRefs; }

private:
	std::vector<UPtr<T>> mPool;
	std::vector<size_t> mFreeIndices;
	std::vector<T*> mActiveRefs;
};