#include "profiler.h"

ProfilerData Profiler::GetData(const str& tag) const
{
	auto it = mData.find(tag);
	if (it != mData.end()) {
		return it->second;
	}
	return {};
}

Duration Profiler::GetTotalTime() const {
	Duration totalTime{ 0.0f };
	for (const auto& [tag, data] : mData) {
		totalTime += data.duration;
	}
	return totalTime;
}

void Profiler::Begin(const str& tag) {
	mData[tag].start = high_resolution_clock::now();
}

void Profiler::End(const str& tag) {
	auto& data = mData[tag];
	data.end = high_resolution_clock::now();
	data.duration = data.end - data.start;
}
