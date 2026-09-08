#pragma once
// #include "../Exceptions.h"
// #include "Util/Vector.h"

#include "Util/Tags.h"

#include <Api/v0/Vector.h>

#include <array>
#include <functional>
#include <map>
#include <string>

namespace Simulation {
	namespace Export {
		namespace Minecraft {
			typedef Util::Serialisation::NBT::Tag Tag;
			typedef Util::Serialisation::NBT::TagUPtr TagUPtr;

			std::vector<uint32_t> unpackLongs(std::vector<int64_t>& data, int bitLength);
			std::vector<int64_t> packLongs(const std::vector<uint32_t>& data, int bitLength);
			std::vector<int64_t> packLongs(const std::array<uint32_t, 4096>& data, int bitLength);

			struct Block {
				std::string name;
				std::map<std::string, std::string> properties;

				Block() = default;
				Block(const std::string& name);
				Block(Tag& tag);
				TagUPtr write();
				bool operator==(const Block& other) const {
					return name == other.name && properties == other.properties;
				}
			};

			struct BlockHasher {
				std::size_t operator()(const Block& block) const {
					std::size_t nameHash = std::hash<std::string>{}(block.name);
					std::size_t propertiesHash = 0;
					
					for (const auto& pair : block.properties) {
						std::size_t keyHash = std::hash<std::string>{}(pair.first);
						std::size_t valueHash = std::hash<std::string>{}(pair.second);
						propertiesHash ^= keyHash + 0x9e3779b9 + (propertiesHash << 6) + (propertiesHash >> 2);
						propertiesHash ^= valueHash + 0x9e3779b9 + (propertiesHash << 6) + (propertiesHash >> 2);
					}
					
					return nameHash ^ (propertiesHash + 0x9e3779b9 + (nameHash << 6) + (nameHash >> 2));
				}
			};

			struct BlockRegistry {
				int idUpto = 0;
				std::unordered_map<int, Block> idToBlock;
				std::unordered_map<Block, int, BlockHasher> blockToId;

				BlockRegistry();
				int getId(const Block& block) const;
				Block getBlock(int id) const;
				int getIdOrRegister(const Block& block);
			};

			struct BlockPalette {
				std::vector<int> blocks;
				const BlockRegistry& blockRegistry;

				BlockPalette(const BlockRegistry& blockRegistry);
				BlockPalette(Tag& tag, BlockRegistry& blockRegistry);
				BlockPalette& operator=(const BlockPalette& other);
				TagUPtr write();

				static std::vector<Block> STANDARD_PALETTE;
				enum StandardPalette{
					DIRT = 0,
					AIR = 1,
					STONE = 2,
					GRASS = 3,
					SHORT_GRASS = 4,
					SAND = 5,
					WATER = 6,
					BEDROCK = 7,
					OAK_LOG = 8,
					OAK_LEAVES = 9,
					DEEPSLATE = 10,
					SANDSTONE = 11,
					JUNGLE_LEAVES = 12,
					SPRUCE_LOG = 13,
					SPRUCE_LEAVES = 14,
					SNOW = 15
				};
			};

			struct Biomes {
				std::vector<std::string> palette;
				std::vector<uint32_t> data;

				Biomes() : palette({ "jungle" }), data({}) {}
				Biomes(Tag& tag);
				TagUPtr write();
				void set(const std::string& name);
			};

			struct BlockStates {
				BlockPalette palette;
				std::array<uint32_t, 4096> data;

				BlockStates(const BlockRegistry& blockRegistry) : palette(blockRegistry) {};
				BlockStates(Tag& tag, BlockRegistry& blockRegistry);
				BlockStates& operator=(const BlockStates& other);
				TagUPtr write();
				std::map<std::string, int> getBlockCounts() const;
			};
			
			struct Section {
				int8_t yPos;
				BlockStates blockStates;
				Biomes biomes;
				std::vector<uint8_t> blockLight;
				std::vector<uint8_t> unpackedSkyLight;

				Section(int8_t yPos, const BlockRegistry& blockRegistry);
				Section(Tag& tag, BlockRegistry& blockRegistry);
				TagUPtr write();
				void setAllBlocks(int block);
				void setBiome(const std::string& biomeName);
				void makeEditable();
				void setLight(int index, uint8_t light);
				inline void setBlock(int index, int blockId);
				inline int getBlockIndex(int blockId);
			};

			struct Heightmaps {
				std::vector<uint32_t> motionBlocking, motionBlockingNoLeaves, oceanFloor, oceanFloorWG, worldSurface, worldSurfaceWG;

				Heightmaps() = default;
				Heightmaps(Tag& tag);
				TagUPtr write();
			};

			struct Structures {
				Structures() = default;
				TagUPtr write();
			};

			struct PostProcessing {
				int numSections = 24;
				TagUPtr write();
			};

			struct Chunk {
				const BlockRegistry& blockRegistry;
				int xPos, yPos, zPos, dataVersion;
				std::string status;
				int64_t lastUpdate, inhabitedTime;
				std::vector<Section> sections;
				int8_t isLightOn;
				Heightmaps heightmaps;
				Structures structures;
				PostProcessing postprocessing;
				int maxHeight = 383;
				int numSections = 24;

				Chunk(const BlockRegistry& blockRegistry, int xPos, int yPos, int zPos, int maxHeight, int dataVersion);
				Chunk(Tag& tag, BlockRegistry& blockRegistry);
				TagUPtr write();
				void setBiome(const std::string& biomeName);
				void setFlatLand();
				void setAllBlocksBetween(tf_v0_ivec2 position, int yFrom, int yTo, int block);
				void setTopBlocksTo(tf_v0_ivec2 position, int air, int block);
				void setIfNotAir(tf_v0_ivec2 position, int y, int air, int block);
				void setLight(tf_v0_ivec2 position, int y, uint8_t light);
			};
		}
	}
}