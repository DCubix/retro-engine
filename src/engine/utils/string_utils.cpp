#include "string_utils.h"

std::vector<str> utils::SplitString(const str& source, const str& delimiter)
{
	std::vector<str> tokens;
	size_t start = 0;
	size_t end = 0;
	while ((end = source.find(delimiter, start)) != str::npos) {
		tokens.push_back(source.substr(start, end - start));
		start = end + delimiter.length();
	}
	tokens.push_back(source.substr(start));
	return tokens;
}

std::vector<str> utils::SplitLines(const str& source)
{
	return SplitString(source, "\n");
}

str utils::JoinStrings(const std::vector<str>& strings, const str& delimiter)
{
	str result;
	for (size_t i = 0; i < strings.size(); ++i) {
		result += strings[i];
		if (i < strings.size() - 1) {
			result += delimiter;
		}
	}
	return result;
}
