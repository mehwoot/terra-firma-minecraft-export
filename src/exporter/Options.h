#pragma once

#include <string>
#include <optional>

namespace Simulation::Export::Minecraft{

enum class MinecraftVersion { V1_20_1, V1_20_2, V1_21_1, V26_1 };

inline constexpr int getDataVersion(MinecraftVersion version) {
	switch (version) {
		case MinecraftVersion::V1_20_1: return 3465;
		case MinecraftVersion::V1_20_2: return 3578;
		case MinecraftVersion::V1_21_1: return 3955;
		case MinecraftVersion::V26_1: return 4786;
		default: return 4786;
	}
}

constexpr inline std::string getVersionName(MinecraftVersion version) {
	switch (version) {
		case MinecraftVersion::V1_20_1: return "1.20.1";
		case MinecraftVersion::V1_20_2: return "1.20.2";
		case MinecraftVersion::V1_21_1: return "1.21.1";
		case MinecraftVersion::V26_1: return "26.1";
		default: return "26.1";
	}
}

constexpr inline auto getVersionByName(auto&& versionString) {
	if (versionString == "1.20.1") {
		return MinecraftVersion::V1_20_1;
	} else if (versionString == "1.20.2") {
		return MinecraftVersion::V1_20_2;
	} else if (versionString == "1.21.1") {
		return MinecraftVersion::V1_21_1;
	} else if (versionString == "26.1") {
		return MinecraftVersion::V26_1;
	} else {
		return MinecraftVersion::V26_1;
	}
}

struct Options{
	int exportMapWidth = 1024;
	std::string filename = "terra firma export";
	std::string folder = "";
	// max height will be calculated if not set
	std::optional<int> maxHeight;
	int seaLevel = 32;
	std::string themeId = "default";
	MinecraftVersion minecraftVersion = MinecraftVersion::V26_1;
};
}
