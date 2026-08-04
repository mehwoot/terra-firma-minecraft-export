
#include "Region.h"
#include "Util/miniz.h"
#include "Util/Compression.h"
#include "Util/Serialisation/NBT/Parser.h"

using namespace Simulation;
using namespace Simulation::Export;
using namespace Simulation::Export::Minecraft;

Region::Region(const BlockRegistry& blockRegistry, tf_v0_ivec2 position, int maxHeight, int dataVersion) : blockRegistry(blockRegistry), position(position), worldRatio(1.f), maxHeight(maxHeight), dataVersion(dataVersion) {
	firstChunkCoordinates = {position.x * 32, position.y*32};
	firstBlockCoordinates = tf_v0_ivec2(position.x*512, position.y*512);

	chunks.reserve(1024);
	for (int z = 0; z < 32; z++) {
		for (int x = 0; x < 32; x++) {
			chunks.emplace_back(blockRegistry, firstChunkCoordinates.x + x, -4, firstChunkCoordinates.y + z, maxHeight, dataVersion);
		}
	}
}

Region::Region(BlockRegistry& blockRegistry, tf_v0_ivec2 position, Buffer& buffer, int dataVersion) : blockRegistry(blockRegistry), worldRatio(1.f), maxHeight(383), dataVersion(dataVersion) {
	std::vector<ChunkMeta> chunkMetaData;
	chunkMetaData.reserve(1024);
	chunks.reserve(1024);

	/* Read in chunk offsets */
	for (int i = 0; i < 1024; i++) {
		uint32_t value1 = buffer.read<uint8_t>(), value2 = buffer.read<uint8_t>(), value3 = buffer.read<uint8_t>();
		uint8_t size = buffer.read<uint8_t>();
		uint32_t offset = (value1 << 16) + (value2 << 8) + value3;
		chunkMetaData.emplace_back(offset, size);
	}

	/* Read in chunk timestamps */
	for (int i = 0; i < 1024; i++) {
		chunkMetaData[i].timestamp = buffer.read<uint32_t>();
	}

	/* Read in chunks */
	for (int i=0; i<1024; i++) {
		auto& chunkMetaDatum = chunkMetaData[i];
		int xPos = i % 32, yPos = -4, zPos = i / 32;
		if (chunkMetaDatum.offset == 0) {
			chunks.emplace_back(blockRegistry, xPos, yPos, zPos, maxHeight, dataVersion);
			continue;
		}

		buffer.setPosition(chunkMetaDatum.offset * 4096);
		int32_t length = buffer.read<int32_t>();
		int8_t compression = buffer.read<int8_t>();
		if (compression != 2) {
			throw FileNotImportable("unsupported compression type");
		}

		unsigned long uncompressedLength = 1024 * 1024;
		std::unique_ptr<char> uncompressedData = std::unique_ptr<char>(new char[uncompressedLength]);
		auto result = mz_uncompress(
			reinterpret_cast<unsigned char*>(uncompressedData.get()), &uncompressedLength, 
			reinterpret_cast<const unsigned char*>(buffer.getCurrentPosition()), uncompressedLength
		);
		if (result != MZ_OK) {
			throw FileNotImportable("zlib decompression failed");
		}

		Util::Serialisation::NBT::Parser parser;
		//parser.setDebugOutput(true);
		Util::Serialisation::ReadBuffer buffer(std::move(uncompressedData), uncompressedLength);
		auto tags = parser.parse(buffer);

		if (tags.size() != 1) {
			throw FileNotImportable("could not parse tags from chunk");
		}
		chunks.emplace_back(*tags.front(), blockRegistry);
		tagsByChunk.try_emplace(tf_v0_ivec2(xPos, zPos), std::move(tags.front()));
	}
}

void Region::setWorldRatio(float worldRatio, ci::ivec2 worldSizeMinecraftUnits) {
	this->worldRatio = worldRatio;
	lvec2 firstBlockMinecraftCoordinates = (position * 512) + lvec2(worldSizeMinecraftUnits.x / 2.f, worldSizeMinecraftUnits.y / 2.f);
	firstBlockWorldCoordinates = wvec2(firstBlockMinecraftCoordinates.x / worldRatio, firstBlockMinecraftCoordinates.y / worldRatio);
}

std::vector<CompressedChunk> Region::compressChunks() {
	std::vector<CompressedChunk> compressedChunks;
	compressedChunks.reserve(1024);

	if (chunks.size() != 1024) {
		throw FileNotImportable("incorrect number of chunk meta data");
	}

	for (auto& chunk : chunks) {
		//if (chunk.valid) {
			Buffer uncompressedWrittenChunk(1024 * 1024);
			chunk.write()->write(uncompressedWrittenChunk);
			uncompressedWrittenChunk.rewind();

			uLong compressedLength = compressBound(static_cast<uLong>(uncompressedWrittenChunk.getWrittenSize()));
			std::unique_ptr<char> compressedData = std::unique_ptr<char>(new char[compressedLength]);

			auto result = mz_compress(
				reinterpret_cast<unsigned char*>(compressedData.get()), &compressedLength,
				reinterpret_cast<const unsigned char*>(uncompressedWrittenChunk.getCurrentPosition()), static_cast<uLong>(uncompressedWrittenChunk.getWrittenSize())
			);

			if (result != MZ_OK) {
				throw FileNotImportable("zlib compression failed");
			}

			compressedChunks.emplace_back(std::move(compressedData), static_cast<uint32_t>(compressedLength), true);
		//} else {
			//compressedChunks.emplace_back(nullptr, 0, false);
		//}
	}

	return compressedChunks;
}

Buffer Region::write() {
	std::vector<CompressedChunk> compressedChunks = compressChunks();
	if (compressedChunks.size() != 1024) {
		throw FileNotImportable("incorrect number of chunk meta data");
	}

	Buffer buffer(1024 * 1024 * 16);
	std::unique_ptr<char> padding = std::unique_ptr<char>(new char[4096 * 16]);
	memset(padding.get(), 0, 4096 * 16);

	std::vector<ChunkMeta> chunkMetaData;
	chunkMetaData.reserve(1024);
	uint32_t blockPosition = 2;

	for (auto& compressedChunk : compressedChunks) {
		if (compressedChunk.valid) {
			buffer.setPosition(blockPosition * 4096);
			uint8_t blocks = static_cast<uint8_t>(std::ceil((5 + compressedChunk.length) / 4096.0));
			buffer.write<uint32_t>(compressedChunk.length);
			buffer.write<uint8_t>(2u);
			buffer.writeBytes(compressedChunk.data.get(), compressedChunk.length);
			uint32_t paddingBytesLength = (blocks * 4096) - (5 + compressedChunk.length);
			buffer.writeBytes(padding.get(), paddingBytesLength);
			chunkMetaData.emplace_back(blockPosition, blocks, 1710905388);
			blockPosition += blocks;
		}
	}

	buffer.setPosition(0);

	for (auto& chunkMetaDatum : chunkMetaData) {
		uint8_t byte1 = chunkMetaDatum.offset >> 16, byte2 = chunkMetaDatum.offset >> 8, byte3 = chunkMetaDatum.offset;
		buffer.write<uint8_t>(byte1);
		buffer.write<uint8_t>(byte2);
		buffer.write<uint8_t>(byte3);
		buffer.write<uint8_t>(chunkMetaDatum.size);
	}

	for (auto& chunkMetaDatum : chunkMetaData) {
		buffer.write<uint32_t>(chunkMetaDatum.timestamp);
	}

	return buffer;
}

void Region::setLighting(tf_v0_ivec2 position, int y, uint8_t light) {
	tf_v0_ivec2 chunkCoordinates = Util::convertLocalToLocalInt(position, 1, 16);
	if (position.x >= 0 && position.y >= 0 && position.x < 512 && position.y < 512) {
		int chunkNumber = (chunkCoordinates.y * 32) + chunkCoordinates.x;
		auto& chunk = chunks[chunkNumber];
		chunk.setLight(tf_v0_ivec2(position.x % 16, position.y % 16), y, light);
	}
}

void Region::setAllBlocksBetween(tf_v0_ivec2 position, int yFrom, int yTo, int block) {
	tf_v0_ivec2 chunkCoordinates = Util::convertLocalToLocalInt(position, 1, 16);
	if (position.x >= 0 && position.y >= 0 && position.x < 512 && position.y < 512) {
		int chunkNumber = (chunkCoordinates.y * 32) + chunkCoordinates.x;
		auto& chunk = chunks[chunkNumber];
		chunk.setAllBlocksBetween(tf_v0_ivec2(position.x % 16, position.y % 16), yFrom, yTo, block);
	}
}

void Region::setTopBlocksTo(tf_v0_ivec2 position, int air, int block) {
	tf_v0_ivec2 chunkCoordinates = Util::convertLocalToLocalInt(position, 1, 16);
	if (position.x >= 0 && position.y >= 0 && position.x < 512 && position.y < 512) {
		int chunkNumber = (chunkCoordinates.y * 32) + chunkCoordinates.x;
		auto& chunk = chunks[chunkNumber];
		chunk.setTopBlocksTo(tf_v0_ivec2(position.x % 16, position.y % 16), air, block);
	}
}

void Region::setIfNotAir(tf_v0_ivec2 position, int y, int air, int block) {
	tf_v0_ivec2 chunkCoordinates = Util::convertLocalToLocalInt(position, 1, 16);
	if (position.x >= 0 && position.y >= 0 && position.x < 512 && position.y < 512) {
		int chunkNumber = (chunkCoordinates.y * 32) + chunkCoordinates.x;
		auto& chunk = chunks[chunkNumber];
		chunk.setIfNotAir(tf_v0_ivec2(position.x % 16, position.y % 16), y, air, block);
	}
}

void Region::setAllBlocksBetween(tf_v0_ivec2 position, int yFrom, int yTo, int block) {
	tf_v0_ivec2 regionCoordinates = tf_v0_ivec2(position) - firstBlockCoordinates;
	if (regionCoordinates.x < 0 || regionCoordinates.y < 0 || regionCoordinates.x >= 512 || regionCoordinates.y >= 512) return;

	setAllBlocksBetween(regionCoordinates, yFrom, yTo, block);
}

void Region::makeFlat() {
	for (auto& chunk : chunks) {
		chunk.setFlatLand();
	}
}

wvec2 Region::convertMinecraftRegionCoordinatesToGameWorldCoordinates(lvec2 regionCoordinates) {
	return firstBlockWorldCoordinates + wvec2(regionCoordinates.x / worldRatio, regionCoordinates.y / worldRatio);
}