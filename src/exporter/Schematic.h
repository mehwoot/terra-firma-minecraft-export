#pragma once

#include "BlockToConstruct.h"
#include "Chunk.h"

#include <list>
#include <filesystem>

namespace Simulation {
	namespace Export {
		namespace Minecraft {
			class Schematic {
			protected:
				std::list<BlockToConstruct> blocks;

			public:
				Schematic(BlockRegistry& blockRegistry, const std::filesystem::path& schematicPath);
				
				const std::list<BlockToConstruct>& getBlocks() const { return blocks; }
			};
		}
	}
}