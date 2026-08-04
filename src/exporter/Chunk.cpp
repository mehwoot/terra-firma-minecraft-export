#include "Chunk.h"
#include <stdexcept>

using namespace Simulation;
using namespace Simulation::Export;
using namespace Simulation::Export::Minecraft;
using namespace Util::Serialisation::NBT;

std::vector<Block> BlockPalette::STANDARD_PALETTE = {
	{"minecraft:dirt"},
	{"minecraft:air"},
	{"minecraft:stone"},
	{"minecraft:grass_block"},
	{"minecraft:short_grass"},
	{"minecraft:sand"},
	{"minecraft:water"},
	{"minecraft:bedrock"},
	{"minecraft:oak_log"},
	{"minecraft:oak_leaves"},
	{"minecraft:deepslate"},
	{"minecraft:sandstone"},
	{"minecraft:jungle_leaves"},
	{"minecraft:spruce_log"},
	{"minecraft:spruce_leaves"},
	{"minecraft:snow_block"},
	{"minecraft:snow"},
	{"minecraft:lava"},
	{"minecraft:grass"},  // 1.20.1 name for short_grass
};

Chunk::Chunk(const BlockRegistry& blockRegistry, int xPos, int yPos, int zPos, int maxHeight, int dataVersion) : blockRegistry(blockRegistry),
		xPos(xPos), yPos(yPos), zPos(zPos), maxHeight(maxHeight) {
	numSections = (maxHeight / 16) + 1;
	postprocessing.numSections = numSections;

	sections.reserve(numSections);
	for (int _yPos = -4; _yPos < (-4 + numSections); _yPos++) {
		sections.emplace_back(_yPos, blockRegistry);
	}
	isLightOn = 1;
	this->dataVersion = dataVersion;
	lastUpdate = 1710915388;
	inhabitedTime = 0;
	status = "minecraft:full";
}

void Chunk::setFlatLand() {
	for (int i = 0; i < 8; i++) {
		sections[i].setAllBlocks(blockRegistry.getId({ "minecraft:stone" }));
	}
}

void Chunk::setAllBlocksBetween(tf_v0_ivec2 position, int yFrom, int yTo, int block) {
	bool newSection = true;
	for (int y = yFrom; y <= yTo; y++) {
		if (y < 0 || y > maxHeight) continue;
		if (y % 16 == 0) {
			newSection = true;
		}
		auto& section = sections[y / 16];
		if (newSection) {
			section.makeEditable();
			newSection = false;
		}
		int index = (((y % 16) * 16) + position.y) * 16 + position.x;
		section.setBlock(index, block);
	}
}

void Chunk::setTopBlocksTo(tf_v0_ivec2 position, int air, int block) {
	bool lastBlockSolid = true;
	uint32_t airId = 0;
	for (int y = 0; y <= maxHeight; y++) {
		auto& section = sections[y / 16];
		if (y % 16 == 0) {
			airId = section.getBlockIndex(air);
			section.makeEditable();
		}
		int index = (((y % 16) * 16) + position.y) * 16 + position.x;

		int blockHere = section.blockStates.data[index];
		if (lastBlockSolid && blockHere == airId) {
			section.setBlock(index, block);
		}
		if (blockHere == airId) {
			lastBlockSolid = false;
		} else {
			lastBlockSolid = true;
		}
	}
}

void Chunk::setIfNotAir(tf_v0_ivec2 position, int y, int air, int block) {
	auto& section = sections[y / 16];
	uint32_t airId = section.getBlockIndex(air);
	section.makeEditable();
	int index = (((y % 16) * 16) + position.y) * 16 + position.x;
	int blockHere = section.blockStates.data[index];
	if (blockHere != airId) {
		section.setBlock(index, block);
	}
}

void Chunk::setLight(tf_v0_ivec2 position, int y, uint8_t light) {
	int index = (((y % 16) * 16) + position.y) * 16 + position.x;
	auto& section = sections[y / 16];
	section.setLight(index, light);
}

Chunk::Chunk(Tag& tag, BlockRegistry& blockRegistry) : blockRegistry(blockRegistry) {
	if (tag.id != 10) {
		throw /*FileNotImportable*/std::runtime_error("Wrong tag type for chunk");
	}

	TagCompound& root = tag.as<TagCompound>();
	for (auto& child : root.children) {
		if (child->name == "DataVersion") {
			dataVersion = child->as<TagInt>().value;
		}
		if (child->name == "xPos") {
			xPos = child->as<TagInt>().value;
		}
		if (child->name == "yPos") {
			yPos = child->as<TagInt>().value;
		}
		if (child->name == "zPos") {
			zPos = child->as<TagInt>().value;
		}
		if (child->name == "Status") {
			status = child->as<TagString>().value;
		}
		if (child->name == "LastUpdate") {
			lastUpdate = child->as<TagLong>().value;
		}
		if (child->name == "InhabitedTime") {
			inhabitedTime = child->as<TagLong>().value;
		}
		if (child->name == "sections") {
			auto& tagList = child->as<TagList>();
			for (auto& sectionTag : tagList.children) {
				sections.push_back(Section(*sectionTag, blockRegistry));
			}
		}
		if (child->name == "Heightmaps") {
			heightmaps = Heightmaps(*child);
		}
		if (child->name == "isLightOn") {
			isLightOn = child->as<TagByte>().value;
		}
	}
}

TagUPtr Chunk::write() {
	std::vector<TagUPtr> children;
	children.push_back(std::make_unique<TagInt>(xPos, "xPos"));
	children.push_back(std::make_unique<TagInt>(yPos, "yPos"));
	children.push_back(std::make_unique<TagInt>(zPos, "zPos"));
	children.push_back(std::make_unique<TagInt>(dataVersion, "DataVersion"));
	children.push_back(std::make_unique<TagLong>(inhabitedTime, "InhabitedTime"));
	children.push_back(std::make_unique<TagLong>(lastUpdate, "LastUpdate"));
	children.push_back(std::make_unique<TagByte>(isLightOn, "isLightOn"));
	children.push_back(std::make_unique<TagString>(status, "Status"));
	children.push_back(heightmaps.write());
	children.push_back(structures.write());
	children.push_back(postprocessing.write());
	children.push_back(std::make_unique<TagList>(std::vector<TagUPtr>{}, "block_entities", 0));
	children.push_back(std::make_unique<TagList>(std::vector<TagUPtr>{}, "fluid_ticks", 0));
	children.push_back(std::make_unique<TagList>(std::vector<TagUPtr>{}, "block_ticks", 0));

	std::vector<TagUPtr> sectionTags;
	for (auto& section : sections) {
		sectionTags.push_back(section.write());
	}
	children.push_back(std::make_unique<TagList>(std::move(sectionTags), "sections", 10));

	return std::make_unique<TagCompound>(std::move(children), "");
}

void Chunk::setBiome(const std::string& biomeName) {
	for (auto& section : sections) {
		section.setBiome(biomeName);
	}
}

TagUPtr Structures::write() {
	std::vector<TagUPtr> children;
	children.push_back(std::make_unique<TagCompound>(std::vector<TagUPtr>{}, "References"));
	children.push_back(std::make_unique<TagCompound>(std::vector<TagUPtr>{}, "starts"));
	return std::make_unique<TagCompound>(std::move(children), "structures");
}

TagUPtr PostProcessing::write() {
	std::vector<TagUPtr> children;
	for (int i = 0; i < numSections; i++) {
		children.push_back(std::make_unique<TagList>(std::vector<TagUPtr>{}, "", 9));
	}
	return std::make_unique<TagList>(std::move(children), "PostProcessing", 9);
}

Section::Section(int8_t yPos, const BlockRegistry& blockRegistry) : blockStates(blockRegistry), yPos(yPos) {

}

Section::Section(Tag& tag, BlockRegistry& blockRegistry) : blockStates(blockRegistry) {
	if (tag.id != 10) {
		throw /*FileNotImportable*/  std::runtime_error("Wrong tag type for section");
	}

	TagCompound& root = tag.as<TagCompound>();
	for (auto& child : root.children) {
		if (child->name == "Y") {
			yPos = child->as<TagByte>().value;
		}
		if (child->name == "block_states") {
			blockStates = BlockStates(*child, blockRegistry);
		}
		if (child->name == "biomes") {
			biomes = Biomes(*child);
		}
		if (child->name == "BlockLight") {
			blockLight = child->as<TagByteArray>().values;
		}
		if (child->name == "SkyLight") {
			// todo: properly unpack skylight values
			//packedSkyLight = child->as<TagByteArray>().values;
		}
	}
}

void Section::setLight(int index, uint8_t light) {
	if(!(index < 4096)) throw std::runtime_error{"invalid index"};
	if (unpackedSkyLight.empty()) {
		unpackedSkyLight = std::vector<uint8_t>(4096, 0xFF);
	}
	unpackedSkyLight[index] = light;
}

TagUPtr Section::write() {
	std::vector<TagUPtr> children;

	children.push_back(std::make_unique<TagByte>(this->yPos, "Y"));
	children.push_back(blockStates.write());
	children.push_back(biomes.write());
	if (!blockLight.empty()) {
		children.push_back(std::make_unique<TagByteArray>(blockLight, "BlockLight"));
	}
	if (!unpackedSkyLight.empty()) {
		std::vector<uint8_t> packedSkyLight(2048);
		for (int i = 0; i < 2048; i++) {
			uint8_t value = 0;
			value |= unpackedSkyLight[i * 2] << 4;
			value |= (unpackedSkyLight[(i * 2) + 1] & 0xF);
			packedSkyLight[i] = value;
		}
		children.push_back(std::make_unique<TagByteArray>(packedSkyLight, "SkyLight"));
	} else {
		children.push_back(std::make_unique<TagStaticByteArray<2048>>(0xFF, "SkyLight"));
	}

	return std::make_unique<TagCompound>(std::move(children), "");
}

void Section::setAllBlocks(int block) {
	blockStates.palette.blocks.clear();
	blockStates.palette.blocks.push_back(block);
}

void Section::setBiome(const std::string& biomeName) {
	biomes.set(biomeName);
}

void Section::makeEditable() {
	if (blockStates.palette.blocks.size() <= 1) {
		std::fill(blockStates.data.begin(), blockStates.data.end(), 0);
	}
}

int Section::getBlockIndex(int blockId) {
	auto indexPos = std::find(blockStates.palette.blocks.begin(), blockStates.palette.blocks.end(), blockId);
	if (indexPos == blockStates.palette.blocks.end()) {
		blockStates.palette.blocks.push_back(blockId);
		indexPos = (blockStates.palette.blocks.end() - 1);
	}
	return indexPos - blockStates.palette.blocks.begin();
}

void Section::setBlock(int index, int block) {
	if(!(index < 4096)) throw std::runtime_error{"invalid index"};
	uint32_t blockId = getBlockIndex(block);
	blockStates.data[index] = blockId;
}

int BlockRegistry::getId(const Block& block) const {
	auto it = blockToId.find(block);
	if (it == blockToId.end()) {
		throw ::Exception(std::format("Block not found in registry: {}", block.name));
	}
	return it->second;
}

BlockRegistry::BlockRegistry() {
	for (auto& block : BlockPalette::STANDARD_PALETTE) {
		getIdOrRegister(block);
	}
	for (int level = 1; level <= 7; level++) {
		Block water;
		water.name = "minecraft:water";
		water.properties["level"] = std::to_string(level);
		getIdOrRegister(water);
	}
}

Block BlockRegistry::getBlock(int id) const {
	auto it = idToBlock.find(id);
	if (it == idToBlock.end()) {
		throw ::Exception(std::format("Block id not found in registry: {}", id));
	}
	return it->second;
}

int BlockRegistry::getIdOrRegister(const Block& block) {
	auto it = blockToId.find(block);
	if (it != blockToId.end()) {
		return it->second;
	}

	int id = idUpto++;
	blockToId.try_emplace(block, id);
	idToBlock.try_emplace(id, block);
	return id;
}

BlockStates& BlockStates::operator=(const BlockStates& other) {
	palette = other.palette;
	std::copy(other.data.begin(), other.data.end(), data.begin());
	return *this;
}

BlockStates::BlockStates(Tag& tag, BlockRegistry& blockRegistry) : palette(blockRegistry) {
	if (tag.id != 10) {
		throw /*FileNotImportable*/  std::runtime_error("Wrong tag type for block states");
	}

	TagCompound& root = tag.as<TagCompound>();
	for (auto& child: root.children) {
		if (child->name == "palette") {
			palette = BlockPalette(*child, blockRegistry);
		}
	}
	for (auto& child : root.children) {
		if (child->name == "data") {
			if (palette.blocks.size() > 0) {
				int paletteBitSize = std::max(4, int(std::ceil(log(palette.blocks.size()) / (log(2.f)))));
				auto _data = unpackLongs(child->as<TagLongArray>().values, paletteBitSize);
				if (_data.size() != data.size()) {
					throw /*FileNotImportable*/  std::runtime_error(std::format("Expected {} blocks in section but found {}", data.size(), _data.size()));
				}
				std::copy(_data.begin(), _data.end(), data.begin());
			}
		}
	}
}

TagUPtr BlockStates::write() {
	std::vector<TagUPtr> children;

	children.push_back(palette.write());
	int paletteBitSize = std::max(4, int(std::ceil(log(palette.blocks.size()) / (log(2.f)))));
	if (palette.blocks.size() == 1) {
		//children.push_back(std::make_unique<TagLongArray>(packLongs(std::vector<uint32_t>(), paletteBitSize), "data"));
	} else {
		children.push_back(std::make_unique<TagLongArray>(packLongs(data, paletteBitSize), "data"));
	}

	return std::make_unique<TagCompound>(std::move(children), "block_states");
}

std::map<std::string, int> BlockStates::getBlockCounts() const {
	std::map<std::string, int> values;
	for (auto& block : palette.blocks) {
		values.emplace(palette.blockRegistry.getBlock(block).name, 0);
	}

	for (auto& value : data) {
		if(!(value < palette.blocks.size(), "out of range");
		values[palette.blockRegistry.getBlock(palette.blocks[value]).name]++;
	}

	return values;
}

BlockPalette::BlockPalette(const BlockRegistry& blockRegistry) : blockRegistry(blockRegistry) {
	blocks = { blockRegistry.getId({ "minecraft:air" }) };
}

BlockPalette& BlockPalette::operator=(const BlockPalette& other) {
	blocks = other.blocks;
	return *this;
}

BlockPalette::BlockPalette(Tag& tag, BlockRegistry& blockRegistry) : blockRegistry(blockRegistry) {
	if (tag.id != 9) {
		throw /*FileNotImportable*/  std::runtime_error("Wrong tag type for block palette");
	}

	TagList& root = tag.as<TagList>();
	blocks.reserve(root.children.size());
	for (auto& child : root.children) {
		Block block(*child);
		blocks.push_back(blockRegistry.getIdOrRegister(block));
	}
}

TagUPtr BlockPalette::write() {
	int8_t childType = 10;

	std::vector<TagUPtr> children;
	for (auto& block : blocks) {
		children.push_back(blockRegistry.getBlock(block).write());
	}

	return std::make_unique<TagList>(std::move(children), "palette", childType);
}

Block::Block(const std::string& name) : name(name) {
	if (name == "minecraft:water") {
		properties = { {"level", "0"} };
	}
}

Block::Block(Tag& tag) {
	if (tag.id != 10) {
		throw /*FileNotImportable*/  std::runtime_error("Wrong tag type for block");
	}

	TagCompound& root = tag.as<TagCompound>();
	for (auto& child : root.children) {
		if (child->name == "Properties") {
			auto& tagCompound = child->as<TagCompound>();
			for (auto& propertyChild : tagCompound.children) {
				auto& tagString = propertyChild->as<TagString>();
				properties.emplace(tagString.name, tagString.value);
			}
		}
		if (child->name == "Name") {
			name = child->as<TagString>().value;
		}
	}
}

TagUPtr Block::write() {
	std::vector<TagUPtr> children;
	children.push_back(std::make_unique<TagString>(name, "Name"));

	std::vector<TagUPtr> propertiesChildren;
	for (auto& pair : properties) {
		propertiesChildren.push_back(std::make_unique<TagString>(pair.second, pair.first));
	}
	if (!propertiesChildren.empty()) {
		children.push_back(std::make_unique<TagCompound>(std::move(propertiesChildren), "Properties"));
	}

	return std::make_unique<TagCompound>(std::move(children), "");
}

Biomes::Biomes(Tag& tag) {
	if (tag.id != 10) {
		throw /*FileNotImportable*/  std::runtime_error("Wrong tag type for biomes");
	}

	TagCompound& root = tag.as<TagCompound>();
	for (auto& child : root.children) {
		if (child->name == "palette") {
			auto& tagList = child->as<TagList>();
			for (auto& tagListChild : tagList.children) {
				palette.push_back(tagListChild->as<TagString>().value);
			}
		}
	}
	for (auto& child : root.children) {
		if (child->name == "data") {
			if (palette.size() > 0) {
				int paletteBitSize = int(std::ceil(log(palette.size()) / (log(2.f))));
				data = unpackLongs(child->as<TagLongArray>().values, paletteBitSize);
			}
		}
	}
}

TagUPtr Biomes::write() {
	std::vector<TagUPtr> children;

	std::vector<TagUPtr> paletteChildren;
	for (auto& name : palette) {
		paletteChildren.push_back(std::make_unique<TagString>(name, "name"));
	}
	children.push_back(std::make_unique<TagList>(std::move(paletteChildren), "palette", 8));

	if (palette.size() > 1) {
		int paletteBitSize = int(std::ceil(log(palette.size()) / (log(2.f))));
		children.push_back(std::make_unique<TagLongArray>(packLongs(data, paletteBitSize), "data"));
	}

	return std::make_unique<TagCompound>(std::move(children), "biomes");
}

void Biomes::set(const std::string& name) {
	palette = { name };
	data = {};
}

Heightmaps::Heightmaps(Tag& tag) {
	if (tag.id != 10) {
		throw /*FileNotImportable*/  std::runtime_error("Wrong tag type for biomes");
	}

	TagCompound& root = tag.as<TagCompound>();
	for (auto& child : root.children) {
		if (child->name == "MOTION_BLOCKING") {
			motionBlocking = unpackLongs(child->as<TagLongArray>().values, 9);
		}
		if (child->name == "MOTION_BLOCKING_NO_LEAVES") {
			motionBlockingNoLeaves = unpackLongs(child->as<TagLongArray>().values, 9);
		}
		if (child->name == "OCEAN_FLOOR") {
			oceanFloor = unpackLongs(child->as<TagLongArray>().values, 9);
		}
		if (child->name == "OCEAN_FLOOR_WG") {
			oceanFloorWG = unpackLongs(child->as<TagLongArray>().values, 9);
		}
		if (child->name == "WORLD_SURFACE") {
			worldSurface = unpackLongs(child->as<TagLongArray>().values, 9);
		}
		if (child->name == "WORLD_SURFACE_WG") {
			worldSurfaceWG = unpackLongs(child->as<TagLongArray>().values, 9);
		}
	}
}

TagUPtr Heightmaps::write() {
	std::vector<TagUPtr> children;

	if (!motionBlocking.empty()) {
		children.push_back(std::make_unique<TagLongArray>(packLongs(motionBlocking, 9), "MOTION_BLOCKING"));
	}
	if (!motionBlockingNoLeaves.empty()) {
		children.push_back(std::make_unique<TagLongArray>(packLongs(motionBlocking, 9), "MOTION_BLOCKING_NO_LEAVES"));
	}
	if (!oceanFloor.empty()) {
		children.push_back(std::make_unique<TagLongArray>(packLongs(motionBlocking, 9), "OCEAN_FLOOR"));
	}
	if (!oceanFloorWG.empty()) {
		children.push_back(std::make_unique<TagLongArray>(packLongs(motionBlocking, 9), "OCEAN_FLOOR_WG"));
	}
	if (!worldSurface.empty()) {
		children.push_back(std::make_unique<TagLongArray>(packLongs(motionBlocking, 9), "WORLD_SURFACE"));
	}
	if (!worldSurfaceWG.empty()) {
		children.push_back(std::make_unique<TagLongArray>(packLongs(motionBlocking, 9), "WORLD_SURFACE_WG"));
	}

	return std::make_unique<TagCompound>(std::move(children), "Heightmaps");
}

std::vector<uint32_t> Minecraft::unpackLongs(std::vector<int64_t>& data, int bitLength) {
	std::vector<uint32_t> values;
	int valuesPerDatum = (64 / bitLength);
	uint32_t mask = (1 << bitLength) - 1;
	int size = valuesPerDatum * (int)data.size();
	values.reserve(size);

	for (int64_t datum : data) {
		int64_t at = 0;
		while (at + bitLength <= 64) {
			uint32_t val = (datum >> at) & mask;
			values.push_back(val);
			at += bitLength;
		}
	}

	return values;
}

std::vector<int64_t> Minecraft::packLongs(const std::vector<uint32_t>& data, int bitLength) {
	int valuesPerDatum = (64 / bitLength);
	uint32_t mask = (1 << bitLength) - 1;

	int size = (int)data.size() / valuesPerDatum;
	if (data.size() % valuesPerDatum != 0) {
		size += 1;
	}
	std::vector<int64_t> values(size);

	int vectorPosition = 0, bitPosition = 0;
	int64_t value = 0;
	for (int64_t datum : data) {
		datum = (datum & mask) << bitPosition;
		value = value | datum;

		bitPosition += bitLength;
		if (bitPosition + bitLength > 64) {
			values[vectorPosition++] = value;
			bitPosition = 0;
			value = 0;
		}
	}

	if (bitPosition != 0) {
		values[vectorPosition] = value;
	}

	return values;
}

std::vector<int64_t> Minecraft::packLongs(const std::array<uint32_t, 4096>& data, int bitLength) {
	int valuesPerDatum = (64 / bitLength);
	uint32_t mask = (1 << bitLength) - 1;

	int size = (int)data.size() / valuesPerDatum;
	if (data.size() % valuesPerDatum != 0) {
		size += 1;
	}
	std::vector<int64_t> values(size);

	int vectorPosition = 0, bitPosition = 0;
	int64_t value = 0;
	for (int64_t datum : data) {
		datum = (datum & mask) << bitPosition;
		value = value | datum;

		bitPosition += bitLength;
		if (bitPosition + bitLength > 64) {
			values[vectorPosition++] = value;
			bitPosition = 0;
			value = 0;
		}
	}

	if (bitPosition != 0) {
		values[vectorPosition] = value;
	}

	return values;
}