#include "HeightDatapack.h"

#include <ostream>
#include <fstream>

static int getPackFormat(MinecraftVersion version) {
	using namespace Simulation::Export::Minecraft;
	switch (version) {
		case MinecraftVersion::V1_20_1: return 15;
		case MinecraftVersion::V1_20_2: return 18;
		case MinecraftVersion::V1_21_1: return 48;
		case MinecraftVersion::V26_1: return 101;
		default: return 101;
	}
}

static bool usesMinMaxFormat(MinecraftVersion version) {
	return version >= MinecraftVersion::V26_1;
}

static std::string formatMCMeta(int packFormat, const std::string& description, MinecraftVersion version) {
	using namespace Simulation::Export::Minecraft;
	if (version == MinecraftVersion::V1_20_1) {
		return std::format(R"<<(
		{{
			"pack": {{
				"pack_format": {},
				"description": [
					{{
						"text": "{}",
						"color": "#6000ff"
					}}
				]
			}}
		}}
	)<<", packFormat, description);
	} else if (usesMinMaxFormat(version)) {
		return std::format(R"<<(
		{{
			"pack": {{
				"min_format": {},
				"max_format": {},
				"description": {{
					"text": "{}",
					"color": "#6000ff"
				}}
			}}
		}}
	)<<", packFormat, packFormat, description);
	} else {
		return std::format(R"<<(
		{{
			"pack": {{
				"supported_formats": {{
					"min_inclusive": {},
					"max_inclusive": {}
				}},
				"pack_format": {},
				"description": {{
					"text": "{}",
					"color": "#6000ff"
				}}
			}}
		}}
	)<<", packFormat, packFormat, packFormat, description);
	}
}

void Simulation::Export::Minecraft::writeMCMeta(const std::filesystem::path& folder, MinecraftVersion version) {
	int packFormat = getPackFormat(version);
	std::string text = formatMCMeta(packFormat, "Higher Heights", version);

	std::filesystem::path metaPath = folder / "pack.mcmeta";
	std::ofstream metaFile(metaPath);
	metaFile << text;
	metaFile.close();
}

void Simulation::Export::Minecraft::writeOverworld(const std::filesystem::path& folder, int maxHeight, MinecraftVersion version) {
	const std::string contents = std::format(R"<<(
		{{
		  "ambient_light": 0,
		  "bed_works": true,
		  "coordinate_scale": 1,
		  "effects": "minecraft:overworld",
		  "skybox": "overworld",
		  "cardinal_light": "default",
		  "has_ceiling": false,
		  "has_raids": true,
		  "has_skylight": true,
		  "height": {},
		  "infiniburn": "#minecraft:infiniburn_overworld",
		  "logical_height": {},
		  "min_y": -64,
		  "monster_spawn_block_light_limit": 0,
		  "monster_spawn_light_level": 0,
		  "natural": true,
		  "piglin_safe": false,
		  "respawn_anchor_works": false,
		  "ultrawarm": false,
		  "attributes": {{}},
		  "timelines": "#minecraft:in_overworld",
		  "default_clock": "overworld",
		  "has_ender_dragon_fight": false
		}}
	)<<", maxHeight, maxHeight);
	std::filesystem::path metaPath = folder / "overworld.json";
	std::ofstream metaFile(metaPath);
	metaFile << contents;
	metaFile.close();
}

void Simulation::Export::Minecraft::writeBiomeDatapack(const std::filesystem::path& folder, float temperature, MinecraftVersion version) {
	int packFormat = getPackFormat(version);

	const std::string biomeContents = std::format(R"<<(
		{{
		  "carvers": {},
		  "downfall": 0.8,
		  "effects": {{
			"fog_color": 12638463,
			"foliage_color": 7842607,
			"grass_color": 9551193,
			"sky_color": 7907327,
			"water_color": 4159204,
			"water_fog_color": 329011
		  }},
		  "attributes": {{
			"minecraft:visual/sky_color": {{ "modifier": "override", "argument": 7907327 }},
			"minecraft:visual/fog_color": {{ "modifier": "override", "argument": 12638463 }},
			"minecraft:visual/water_fog_color": {{ "modifier": "override", "argument": 329011 }}
		  }},
		  "features": [],
		  "has_precipitation": true,
		  "spawn_costs": {{}},
		  "spawners": {{}},
		  "temperature": {}
		}}
	)<<", version >= MinecraftVersion::V26_1 ? "[]" : "{}", temperature);
	std::filesystem::path biomePath = folder / "datapacks" / "custombiome" / "data" / "custombiome" / "worldgen" / "biome";
	if (!std::filesystem::exists(biomePath)) {
		std::filesystem::create_directories(biomePath);
	}
	std::ofstream biomeFile(biomePath / "default.json");
	biomeFile << biomeContents;
	biomeFile.close();

	std::string metaContents = formatMCMeta(packFormat, "Terra Firma Custom Biomes", version);

	std::ofstream metaFile(folder / "datapacks" / "custombiome" / "pack.mcmeta");
	metaFile << metaContents;
	metaFile.close();
}
