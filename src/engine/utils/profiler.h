#pragma once

#include "custom_types.h"

#include <chrono>
#include <unordered_map>

using namespace std::chrono;

using Duration = duration<float, std::milli>;
using TimePoint = high_resolution_clock::time_point;

struct RENGINE_API ProfilerData {
	TimePoint start;
	TimePoint end;
	Duration duration;
};

class RENGINE_API Profiler {
	using Data = std::unordered_map<str, ProfilerData>;
public:
	Profiler() = default;
	~Profiler() = default;

	void Begin(const str& tag);
	void End(const str& tag);

	Duration GetTotalTime() const;
	ProfilerData GetData(const str& tag) const;

	const Data& GetData() const { return mData; }

private:
	Data mData;
};