
#include "Theme.h"
#include "Chunk.h"
#include "Schematic.h"
#include "Util/Parser.h"
#include "Util/Strings.h"
#include "mc-export-plugin-module.h"

using namespace Simulation;
using namespace Simulation::Export;
using namespace Simulation::Export::Minecraft;
using namespace Util::Serialisation;

PlantDefinition PlantDefinition::deserialise(Util::Serialisation::NBT::TagCompound& root) {
	PlantDefinition value;
	value.name = root.name;

	for (auto& child : root.children) {
		if (child->name == "schematic") {
			value.schematicPath = child->as<NBT::TagString>().value;
		} else if (child->name == "single_block") {
			value.blockName = child->as<NBT::TagString>().value;
		} else if (child->name == "folder") {
			value.folderPath = child->as<NBT::TagString>().value;
		} else {
			reportFatalErrorC(std::format("Plant {} does not have a schematic or single block defined", value.name).c_str());
		}
	}

	return value;
}

void RegionBiomes::setBiome(const tf_v0_ivec2& position, BiomeInstance biome) {
	biomes.emplace(getIndex(position), std::move(biome));
}

const BiomeInstance& RegionBiomes::getBiome(const tf_v0_ivec2& position) const {
	int index = getIndex(position);
	auto biomeIt = biomes.find(index);
	if (biomeIt != biomes.end()) {
		return biomeIt->second;
	}
	reportFatalErrorC("biome missing");
}

int RegionBiomes::getIndex(const tf_v0_ivec2& position) const {
	return std::clamp(((position.y / 16) * 32) + (position.x / 16), 0, 1023);
}

Theme::Theme(const std::filesystem::path& rootFolder) : rootFolder(rootFolder) {
	schematicsFolder = rootFolder / "schematics";

	auto rootTag = Util::Serialisation::loadTagFromFile(rootFolder / "config.nbt");
	load(rootTag->as<Util::Serialisation::NBT::TagCompound>());
}

void Theme::loadPlantDefinitions(const Util::Serialisation::NBT::TagUPtr& tag) {
	auto& plantsTag = tag->as<NBT::TagCompound>();
	for (auto& plant : plantsTag.children) {
		auto plantDefinition = PlantDefinition::deserialise(plant->as<NBT::TagCompound>());
		plantDefinition.id = plantIdUpto++;
		if (plantDefinition.schematicPath) {
			plantDefinition.schematicPath = schematicsFolder / plantDefinition.schematicPath.value();
		}
		if (plantDefinition.folderPath) {
			plantDefinition.folderPath = schematicsFolder / plantDefinition.folderPath.value();
		}
		plantDefinitions.emplace(plantDefinition.name, std::move(plantDefinition));
	}
}

void Theme::loadGenerators(BlockRegistry& blockRegistry) {
	for (auto& [name, plantDefinition] : plantDefinitions) {
		if (plantDefinition.schematicPath.has_value()) {
			auto& schematicPath = plantDefinition.schematicPath.value();
			if (!std::filesystem::exists(schematicPath)) {
				reportFatalErrorC(std::format("Plant schematic file not found: {}", Util::safeString(schematicPath)).c_str());
			}
			Schematic schematic(blockRegistry, schematicPath);
			placeables.emplace(plantDefinition.id, std::make_unique<Placeable>(plantDefinition.name, schematic.getBlocks()));
		} else if (plantDefinition.blockName.has_value()) {
			int blockId = blockRegistry.getIdOrRegister(Block(plantDefinition.blockName.value()));
			placeables.emplace(plantDefinition.id, std::make_unique<Placeable>(plantDefinition.name, std::list<BlockToConstruct>{ {blockId, {0, 0, 0}} } ));
		} else if (plantDefinition.folderPath.has_value()) {
			auto& folderPath = plantDefinition.folderPath.value();
			if (!std::filesystem::exists(folderPath) || !std::filesystem::is_directory(folderPath)) {
				reportFatalErrorC(std::format("Plant folder not found: {}", Util::safeString(folderPath)).c_str());
			}
			std::vector<Placeable> children;
			children.reserve(16);
			for (const auto& entry : std::filesystem::directory_iterator(folderPath)) {
				if (entry.is_regular_file() && entry.path().extension() == ".schem") {
					Schematic schematic(blockRegistry, entry.path());
					children.emplace_back(entry.path().stem().string(), schematic.getBlocks());
				}
			}
			if (children.size() == 0) {
				reportFatalErrorC(std::format("No .schematic files found in plant folder: {}", Util::safeString(folderPath)).c_str());
			}
			placeables.emplace(plantDefinition.id, std::make_unique<CollectionPlaceable>(plantDefinition.name, std::move(children)));
		} else {
			reportFatalErrorC(std::format("Plant definition {} has no generator", name).c_str());
		}
	}

	for (auto& [name, oreDefinition] : oreDefinitions) {
		if (!oreDefinition.schematicFolder.has_value()) {
			reportFatalErrorC(std::format("Ore definition {} has no schematic folder", name).c_str());
		}
		auto folderPath = schematicsFolder / oreDefinition.schematicFolder.value();
		if (!std::filesystem::exists(folderPath) || !std::filesystem::is_directory(folderPath)) {
			reportFatalErrorC(std::format("Ore schematic folder not found: {}", Util::safeString(folderPath)).c_str());
		}
		std::vector<Placeable> children;
		children.reserve(16);
		for (const auto& entry : std::filesystem::directory_iterator(folderPath)) {
			if (entry.is_regular_file() && entry.path().extension() == ".schem") {
				Schematic schematic(blockRegistry, entry.path());
				children.emplace_back(entry.path().stem().string(), schematic.getBlocks());
				children.back().setReplaceAir(false, blockRegistry.getId({ "minecraft:air" }));
			}
		}
		if (children.size() == 0) {
			reportNonFatalErrorC(std::format("No .schematic files found in ore folder: {}", Util::safeString(folderPath)).c_str());
			continue;
		}
		int placeableId = plantIdUpto++;
		placeables.emplace(placeableId, std::make_unique<CollectionPlaceable>(oreDefinition.name, std::move(children)));
		ores.emplace_back(
			Ore::create(placeableId, oreDefinition.rockDepthDistribution.get(), oreDefinition.worldYPositionDistribution.get())
		);
	}
}

std::list<BiomeDefinition::Plant> Theme::deserialisePlants(Util::Serialisation::NBT::TagCompound& root) const {
	std::list<BiomeDefinition::Plant> plants;
	float probabilityLimit = 0.f;
	for (auto& biomeTreeTag : root.children) {
		std::string templateName;
		std::string name = biomeTreeTag->name;

		for (auto& treeChild : biomeTreeTag->as<NBT::TagCompound>().children) {
			if (treeChild->name == "probability") {
				probabilityLimit += treeChild->as<NBT::TagFloat>().value;
			}
			if (treeChild->name == "template") {
				templateName = treeChild->as<NBT::TagString>().value;
			}
		}
		if (templateName.size() == 0) {
			reportFatalErrorC(std::format("Tree {} missing template", name).c_str());
		}
		auto idIt = plantDefinitions.find(templateName);
		if (idIt == plantDefinitions.end()) {
			reportFatalErrorC(std::format("Tree template {} not found in biome", templateName).c_str());
		}
		plants.emplace_back(idIt->second.id, probabilityLimit);
	}
	return plants;
}

void Theme::loadBiomes(const Util::Serialisation::NBT::TagUPtr& tag) {
	auto& biomesTag = tag->as<NBT::TagCompound>();
	for (auto& biome : biomesTag.children) {
		std::string name = biome->name;
		std::string minecraftId;
		std::list<BiomeDefinition::Plant> trees, shrubs;

		auto& biomeTag = biome->as<NBT::TagCompound>();
		for (auto& biomeChildTag : biomeTag.children) {
			if (biomeChildTag->name == "trees") {
				trees = deserialisePlants(biomeChildTag->as<NBT::TagCompound>());
			}
			if (biomeChildTag->name == "shrubs") {
				shrubs = deserialisePlants(biomeChildTag->as<NBT::TagCompound>());
			}
			if (biomeChildTag->name == "minecraft_id") {
				minecraftId = biomeChildTag->as<NBT::TagString>().value;
			}
		}

		if (minecraftId.empty()) {
			reportFatalErrorC(std::format("Biome {} missing minecraft_id", name).c_str());
		}

		biomeDefinitions.emplace(name, BiomeDefinition{ std::move(trees), std::move(shrubs), name, minecraftId });
	}
}

void Theme::loadOreDefinitions(const Util::Serialisation::NBT::TagUPtr& tag) {
	auto& oresTag = tag->as<NBT::TagCompound>();
	for (auto& oreTag : oresTag.children) {
		auto oreDefinition = OreDefinition::deserialise(oreTag->as<NBT::TagCompound>());
		oreDefinitions.emplace(oreDefinition.name, std::move(oreDefinition));
	}
}

void Theme::load(NBT::TagCompound& biomeConfig) {
	auto& root = biomeConfig;
	auto plantsChildIt = std::find_if(root.children.begin(), root.children.end(), [](const NBT::TagUPtr& child) { return child->name == "plants"; });
	if (plantsChildIt == root.children.end()) {
		reportNonFatalErrorC("Parsing minecraft export config failed - no plants tag under root");
	}
	loadPlantDefinitions(*plantsChildIt);

	auto biomesChildIt = std::find_if(root.children.begin(), root.children.end(), [](const NBT::TagUPtr& child) { return child->name == "biomes"; });
	if (biomesChildIt == root.children.end()) {
		reportNonFatalErrorC("Parsing minecraft export config failed - no biomes tag under root");
	}
	loadBiomes(*biomesChildIt);

	auto oresChildIt = std::find_if(root.children.begin(), root.children.end(), [](const NBT::TagUPtr& child) { return child->name == "ores"; });
	if (oresChildIt != root.children.end()) {
		loadOreDefinitions(*oresChildIt);
	}
}

const BiomeDefinition& Theme::getBiomeDefinition(const std::string& name) const {
	auto biomeDefinitionIt = biomeDefinitions.find(name);
	if (biomeDefinitionIt != biomeDefinitions.end()) {
		return biomeDefinitionIt->second;
	}
	reportFatalErrorC(std::format("biome definition {} missing", name).c_str());
}

void Theme::generate(Region& region, const Placement& placement) const {
	auto placeableIt = placeables.find(placement.placeableId);
	if (placeableIt == placeables.end()) {
		reportFatalErrorC(std::format("placeable {} missing", placement.placeableId).c_str());
	}
	placeableIt->second->generate(region, placement.position, placement.randSeed);
}

void Theme::copyDatapackFiles(const std::filesystem::path& worldFolder) const {
	std::filesystem::path destinationDatapackFolder = worldFolder / "datapacks";
	std::filesystem::path sourceDatapackFolder = rootFolder / "datapacks";

	if (!std::filesystem::exists(sourceDatapackFolder)) {
		return;
	}

	if (!std::filesystem::exists(destinationDatapackFolder)) {
		if (!std::filesystem::create_directories(destinationDatapackFolder)) {
			reportFatalErrorC("Could not create datapack folder");
		}
	}

	for (const auto& entry : std::filesystem::recursive_directory_iterator(sourceDatapackFolder)) {
		if (entry.is_regular_file()) {
			std::filesystem::path relativePath = std::filesystem::relative(entry, sourceDatapackFolder);
			std::filesystem::path destination = destinationDatapackFolder / relativePath;
			std::filesystem::create_directories(destination.parent_path());
			std::filesystem::copy_file(entry, destination, std::filesystem::copy_options::overwrite_existing);
		}
	}
}

std::optional<int> Theme::shouldPlaceOreAt(int rockDepth, int worldYPosition, int randSeed) const {
	for (const auto& ore : ores) {
		auto placeableId = ore.shouldPlaceAt(rockDepth, worldYPosition, randSeed);
		if (placeableId.has_value()) {
			return placeableId;
		}
	}
	return std::nullopt;
}

/* ====================== ThemeDefinition ====================== */

Theme ThemeDefinition::loadTheme() const {
	return Theme(folder);
}
