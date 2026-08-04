#pragma once

// #include "Util/Singleton.h"
#include "Trees.h"
#include "Chunk.h"
// #include "Util/Serialisation/NBT/Parser.h"
#include "Ore.h"

namespace Simulation {
	namespace Export {
		namespace Minecraft {
			struct Placement {
				int placeableId;
				tf_v0_ivec3 position;
				int randSeed;
			};

			struct BiomeDefinition {
				struct Plant {
					int id;
					float probabilityLimit;
				};

				std::list<Plant> trees, shrubs;
				std::string name;
				std::string minecraftResourceLocation;
			};

			// A plant can either be
			//	- a single schematic (schematicPath)
			//	- a folder of schematics that will be randomly chosen from (folderPath)
			//  - a single block that will be placed (blockName)
			struct PlantDefinition {
				std::string name;
				int id;
				std::optional<std::filesystem::path> schematicPath;
				std::optional<std::filesystem::path> folderPath;
				std::optional<std::string> blockName;

				static PlantDefinition deserialise(Util::Serialisation::NBT::TagCompound& root);
			};

			struct BiomeInstance {
				const BiomeDefinition& definition;
				float treeCoverage;
			};

			class RegionBiomes {
			protected:
				std::unordered_map<int, BiomeInstance> biomes;

				int getIndex(const tf_v0_ivec2& position) const;

			public:
				void setBiome(const tf_v0_ivec2& position, BiomeInstance biome);
				const BiomeInstance& getBiome(const tf_v0_ivec2& position) const;
			};

			class Theme {
			protected:
				int plantIdUpto = 0;
				std::filesystem::path rootFolder, schematicsFolder;
				std::map<std::string, BiomeDefinition> biomeDefinitions;
				std::map<std::string, PlantDefinition> plantDefinitions;
				std::map<std::string, OreDefinition> oreDefinitions;

				std::map<int, PlaceableUPtr> placeables;
				std::list<Ore> ores;

				void loadOreDefinitions(const Util::Serialisation::NBT::TagUPtr& tag);
				void loadPlantDefinitions(const Util::Serialisation::NBT::TagUPtr& tag);
				void loadBiomes(const Util::Serialisation::NBT::TagUPtr& tag);
				std::list<BiomeDefinition::Plant> deserialisePlants(Util::Serialisation::NBT::TagCompound& root) const;
				void load(Util::Serialisation::NBT::TagCompound& biomeConfig);

			public:
				Theme(const std::filesystem::path& rootFolder);

				const BiomeDefinition& getBiomeDefinition(const std::string& name) const;
				void generate(Region& region, const Placement& placement) const;
				void copyDatapackFiles(const std::filesystem::path& worldFolder) const;
				void loadGenerators(BlockRegistry& blockRegistry);
				std::optional<int> shouldPlaceOreAt(int rockDepth, int worldYPosition, int randSeed) const;
			};

			struct ThemeDefinition {
				std::string displayName;
				bool displayNameIsTranslationKey;
				std::string id;
				std::filesystem::path folder;

				Theme loadTheme() const;
			};
		}
	}
}