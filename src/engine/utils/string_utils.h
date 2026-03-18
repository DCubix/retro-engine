#pragma once

#include "custom_types.h"
#include <vector>

namespace utils {
	std::vector<str> RENGINE_API SplitString(const str& source, const str& delimiter);
	std::vector<str> RENGINE_API SplitLines(const str& source);
	str RENGINE_API JoinStrings(const std::vector<str>& strings, const str& delimiter);
}