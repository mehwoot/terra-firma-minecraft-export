#pragma once

#include "Options.h"

#include <filesystem>

namespace Simulation {
	namespace Export {
		namespace Minecraft {
			void writeMCMeta(const std::filesystem::path& folder, MinecraftVersion version);
			void writeOverworld(const std::filesystem::path& folder, int maxHeight, MinecraftVersion version);
			void writeBiomeDatapack(const std::filesystem::path& folder, float temperature, MinecraftVersion version);
		}
	}
}