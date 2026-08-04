#pragma once
// #include "Util/Parser.h"
#include "Util/Distribution.h"
#include "Util/Random.h"

#include <array>

namespace Simulation {
	namespace Export {
		namespace Minecraft {
			constexpr int ORE_SPACING = 16;

			struct OreDefinition {
				std::string name;
				std::optional<std::filesystem::path> schematicFolder;
				Util::DistributionUPtr rockDepthDistribution = nullptr, worldYPositionDistribution = nullptr;

				static OreDefinition deserialise(Util::Serialisation::NBT::TagCompound& root);
			};

			class Ore {
			protected:
				int placeableId;
				std::array<float, 384> rockDepthDistribution, worldYPositionDistribution;
				mutable Util::DeterministicRandom randomDistribution;

			public:
				Ore(int placeableId, std::array<float, 384> rockDepthDistribution, std::array<float, 384> worldYPositionDistribution);

				std::optional<int> shouldPlaceAt(int rockDepth, int worldYPosition, int randSeed) const;
				static Ore create(int placeableId, Util::Distribution* rockDepthDistribution, Util::Distribution* worldYPositionDistribution);
			};
		}
	}
}