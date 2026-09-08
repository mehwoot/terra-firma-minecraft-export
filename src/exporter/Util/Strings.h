#pragma once

#include <vector>
#include <string>
#include <array>
#include <algorithm>
#include <filesystem>

namespace Util {
	template<int size>
	std::vector<std::string> convertToVector(const std::array<std::string_view, size>& input) {
		std::vector<std::string> values;
		values.reserve(size); 
		for (auto& value : input) {
			values.emplace_back(value);
		}
		return values;
	}

	inline const std::string& transformToTitleCase(std::string& input) {
		std::transform(input.begin() + 1, input.end(), input.begin() + 1, ::tolower);
		return input;
	}

	inline std::string safeString(const std::filesystem::path& path) {
		auto u8path = path.u8string();
		return std::string(reinterpret_cast<const char*>(u8path.c_str()), u8path.size());
	}
}