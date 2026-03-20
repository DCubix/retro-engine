#pragma once

#include "custom_types.h"
#include <vector>

namespace utils {
	std::vector<str> SplitString(const str& source, const str& delimiter);
	std::vector<str> SplitLines(const str& source);
	str JoinStrings(const std::vector<str>& strings, const str& delimiter);
}