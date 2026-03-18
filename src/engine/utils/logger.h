#pragma once

#include "custom_types.h"

#include <fmt/color.h>
#include <fmt/format.h>

#include <stdexcept>

namespace utils {
	template <typename... Args>
	inline void LogE(const std::string& message, Args&&...args) {
		fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::red), "[ERROR] ");
		fmt::print(fmt::fg(fmt::color::white), fmt::runtime(message), args...);
		fmt::print("\n");
	}

	template <typename... Args>
	inline void LogW(const std::string& message, Args&&...args) {
		fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::yellow), "[WARNING] ");
		fmt::print(fmt::fg(fmt::color::white), fmt::runtime(message), args...);
		fmt::print("\n");
	}

	template <typename... Args>
	inline void LogI(const std::string& message, Args&&...args) {
		fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::steel_blue), "[INFO] ");
		fmt::print(fmt::fg(fmt::color::white), fmt::runtime(message), args...);
		fmt::print("\n");
	}

	template <typename... Args>
	inline void LogD(const std::string& message, Args&&...args) {
		fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::green), "[DEBUG] ");
		fmt::print(fmt::fg(fmt::color::white), fmt::runtime(message), args...);
		fmt::print("\n");
	}

	template <typename... Args>
	inline void Log(const std::string& message, Args&&...args) {
		fmt::print(fmt::fg(fmt::color::white), fmt::runtime(message), args...);
		fmt::print("\n");
	}

	template <typename... Args>
	inline void Fail(const std::string& message, Args&&...args) {
		fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::red), "[FAIL] ");
		fmt::print(fmt::fg(fmt::color::white), fmt::runtime(message), args...);
		fmt::print("\n");
		// fail 
	}
}
