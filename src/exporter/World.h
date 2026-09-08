#pragma once
#include "Options.h"
#include "Util/Buffer.h"
#include "Util/Tags.h"

namespace Simulation {
	namespace Export {
		namespace Minecraft {
			class World {
			protected:
				std::string name;
				int spawnHeight;
				MinecraftVersion minecraftVersion;

			public:
				World(const std::string& name, int spawnHeight, MinecraftVersion version);
				~World() = default;

				Buffer write();
				Util::Serialisation::NBT::TagCompoundUPtr serialiseWorldGenSettings();
			};
		}
	}
}