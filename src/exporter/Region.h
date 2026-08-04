#pragma once
#include "Chunk.h"

#include <memory>

namespace Simulation {
	namespace Export {
		namespace Minecraft {
			struct ChunkMeta {
				uint32_t offset;
				uint8_t size;
				uint32_t timestamp;
			};

			struct CompressedChunk {
				std::unique_ptr<char> data;
				uint32_t length;
				bool valid = false;
			};

			struct Region {
				const BlockRegistry& blockRegistry;
				std::vector<Chunk> chunks;
				std::map<tf_v0_ivec2, Util::Serialisation::NBT::TagUPtr> tagsByChunk;
				tf_v0_ivec2 position;
				tf_v0_ivec2 firstChunkCoordinates;
				tf_v0_ivec2 firstBlockCoordinates;
				tf_v0_vec2 firstBlockWorldCoordinates;
				float worldRatio;
				int maxHeight;
				int dataVersion;

				Region(const BlockRegistry& blockRegistry, tf_v0_ivec2 position, int maxHeight, int dataVersion);
				Region(BlockRegistry& blockRegistry, tf_v0_ivec2 position, Buffer& buffer, int dataVersion);

				Buffer write();
				std::vector<CompressedChunk> compressChunks();
				void setAllBlocksBetweenLocal(tf_v0_ivec2 position, int yFrom, int yTo, int block);
				void setAllBlocksBetweenWorld(tf_v0_ivec2 position, int yFrom, int yTo, int block);
				// set first air block above a non air block to something
				void setTopBlocksTo(tf_v0_ivec2 position, int air, int block);
				void setIfNotAir(tf_v0_ivec2 position, int y, int air, int block);
				void setLighting(tf_v0_ivec2 position, int y, uint8_t light);
				void makeFlat();
				void setWorldRatio(float worldRatio, tf_v0_ivec2 worldSizeMinecraftUnits);
				tf_v0_vec2 convertMinecraftRegionCoordinatesToGameWorldCoordinates(tf_v0_vec2 regionCoordinates);
			};
		}
	}
}