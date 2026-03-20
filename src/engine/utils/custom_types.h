#pragma once
#include <memory>
#include <cstdint>
#include <string>
#include <tuple>
#include <optional>

template <typename T>
using UPtr = std::unique_ptr<T>;

template <typename T>
using SPtr = std::shared_ptr<T>;

template <typename T>
using WPtr = std::weak_ptr<T>;

template <typename T>
using Opt = std::optional<T>;

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;
using f32 = float;
using f64 = double;

using byte = u8;

using str = std::string;
using cstr = const char*;

template <typename... Args>
using Tup = std::tuple<Args...>;

struct alignas(8) TextureHandle {
	u32 x, y;

	TextureHandle() : x(0), y(0) {}
	TextureHandle(u64 val)
		: x(val & 0xFFFFFFFFu), y((val >> 32) & 0xFFFFFFFFu)
	{}
	TextureHandle& operator=(u64 val) {
		x = val & 0xFFFFFFFFu;
		y = (val >> 32) & 0xFFFFFFFFu;
		return *this;
	}
};
