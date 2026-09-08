#include "Schematic.h"

#include "Util/Parser.h"
#include "Util/Compression.h"
#include "exporter/Util/Compression.h"
#include "mc-export-plugin-module.h"

using namespace Simulation::Export::Minecraft;
using namespace Util::Serialisation::NBT;

std::vector<uint32_t> decodeVarIntArray(const std::vector<uint8_t>& data) {
	std::vector<uint32_t> result;
	size_t i = 0;
	
	while (i < data.size()) {
		uint32_t value = 0;
		int shift = 0;
		uint8_t byte;
		
		do {
			if (i >= data.size()) break;
			byte = data[i++];
			value |= (byte & 0x7F) << shift;
			shift += 7;
		} while ((byte & 0x80) != 0 && shift < 32);
		
		result.push_back(value);
	}
	
	return result;
}

Schematic::Schematic(BlockRegistry& blockRegistry, const std::filesystem::path& schematicPath) {
	::Buffer buffer(schematicPath);
	auto decompressedData = Util::Serialisation::gzipDecompress(buffer);
	::Util::Serialisation::ReadBuffer readBuffer(std::move(decompressedData));
	// Load the NBT file
	auto tags = Util::Serialisation::NBT::Parser().parse(readBuffer);
	if (tags.size() != 1) {
		reportFatalErrorC("Invalid schematic file - should have one root tag");
	}
	auto& root = tags.front()->as<Util::Serialisation::NBT::TagCompound>();
	
	// Parse schematic metadata
	int version = 0, dataVersion = 0;
	uint16_t width = 0, height = 0, length = 0;
	std::array<int32_t, 3> offset = {0, 0, 0};
	int paletteMax = 0;
	
	// Parse palette (mapping of block state strings to indices)
	std::vector<Block> palette;
	std::vector<uint32_t> blockData;
	
	for (auto& child : root.children) {
		if (child->name == "Version") {
			version = child->as<TagInt>().value;
		} else if (child->name == "DataVersion") {
			dataVersion = child->as<TagInt>().value;
		} else if (child->name == "Width") {
			width = (uint16_t)child->as<TagShort>().value;
		} else if (child->name == "Height") {
			height = (uint16_t)child->as<TagShort>().value;
		} else if (child->name == "Length") {
			length = (uint16_t)child->as<TagShort>().value;
		} else if (child->name == "Offset") {
			//auto& offsetArray = child->as<TagIntArray>();
			//if (offsetArray.values.size() >= 3) {
			//	offset[0] = offsetArray.values[0];
			//	offset[1] = offsetArray.values[1]; 
			//	offset[2] = offsetArray.values[2];
			//}
		} else if (child->name == "PaletteMax") {
			paletteMax = child->as<TagInt>().value;
		} else if (child->name == "Palette") {
			// Parse palette compound - keys are block state strings, values are indices
			auto& paletteCompound = child->as<TagCompound>();
			palette.resize(paletteCompound.children.size());
			
			for (auto& paletteEntry : paletteCompound.children) {
				int index = paletteEntry->as<TagInt>().value;
				if (index >= 0 && index < palette.size()) {
					// Parse block state string (format: "minecraft:block_name[property=value,...]")
					std::string blockState = paletteEntry->name;
					size_t bracketPos = blockState.find('[');
					
					Block block;
					if (bracketPos != std::string::npos) {
						// Has properties
						block.name = blockState.substr(0, bracketPos);
						std::string propertiesStr = blockState.substr(bracketPos + 1);
						propertiesStr = propertiesStr.substr(0, propertiesStr.length() - 1); // Remove closing ]
						
						// Parse properties
						size_t pos = 0;
						while (pos < propertiesStr.length()) {
							size_t equalsPos = propertiesStr.find('=', pos);
							size_t commaPos = propertiesStr.find(',', pos);
							if (commaPos == std::string::npos) commaPos = propertiesStr.length();
							
							if (equalsPos != std::string::npos && equalsPos < commaPos) {
								std::string key = propertiesStr.substr(pos, equalsPos - pos);
								std::string value = propertiesStr.substr(equalsPos + 1, commaPos - equalsPos - 1);
								block.properties[key] = value;
							}
							
							pos = commaPos + 1;
						}
					} else {
						// No properties
						block.name = blockState;
					}
					
					palette[index] = block;
				}
			}
		} else if (child->name == "BlockData") {
			// Decode varint array
			auto& byteArray = child->as<TagByteArray>();
			blockData = decodeVarIntArray(byteArray.values);
		} else if (child->name == "Metadata") {
			auto& metadata = child->as<TagCompound>();
			for (auto& metaChild : metadata.children) {
				if (metaChild->name == "WEOffsetX") {
					offset[0] = metaChild->as<TagInt>().value;
				} else if (metaChild->name == "WEOffsetY") {
					offset[1] = metaChild->as<TagInt>().value;
				} else if (metaChild->name == "WEOffsetZ") {
					offset[2] = metaChild->as<TagInt>().value;
				}
			}
		}
	}
	
	// Validate we have required data
	if (version != 2) {
		reportFatalErrorC("Unsupported schematic version - only version 2 supported");
	}
	if (width == 0 || height == 0 || length == 0) {
		reportFatalErrorC("Invalid schematic dimensions");
	}
	if (palette.empty() || blockData.empty()) {
		reportFatalErrorC("Missing palette or block data");
	}
	
	// Expected block data size should be width * height * length
	size_t expectedSize = (size_t)width * height * length;
	if (blockData.size() != expectedSize) {
		reportFatalErrorC(std::format("Block data size {} doesn't match expected size {}", blockData.size(), expectedSize).c_str());
	}
	
	// Convert blocks to registry IDs and store positions
	blocks.clear();
	for (size_t i = 0; i < blockData.size(); i++) {
		uint32_t paletteIndex = blockData[i];
		if (paletteIndex >= palette.size()) {
			reportFatalErrorC(std::format("Invalid palette index {} (max {})", paletteIndex, palette.size() - 1).c_str());
		}
		
		const Block& block = palette[paletteIndex];
		if (block.name == "minecraft:air") {
			continue;
		}
		
		// Calculate 3D position from linear index (x + z * Width + y * Width * Length)
		int x = i % width;
		int z = (i / width) % length;
		int y = static_cast<int>(i) / (width * length);
		
		// Apply offset
		x += offset[0];
		y += offset[1];
		z += offset[2];
		
		// Register block with registry and store
		int blockId = blockRegistry.getIdOrRegister(block);
		blocks.push_back({ blockId, tf_v0_ivec3(x, y, z) });
	}
}
