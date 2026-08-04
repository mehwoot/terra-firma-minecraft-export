#include "Includes.h"
#include "World.h"
#include "Util/Serialisation/Compression.h"
#include "Util/Serialisation/NBT/Tags.h"

using namespace Simulation;
using namespace Simulation::Export;
using namespace Simulation::Export::Minecraft;
using namespace Util::Serialisation::NBT;

World::World(const std::string& name, int spawnHeight, MinecraftVersion version)
	: name(name), spawnHeight(spawnHeight), minecraftVersion(version) {

}

Buffer World::write() {
	int dataVersion = getDataVersion(minecraftVersion);
	std::string versionName = getVersionName(minecraftVersion);
	bool is26Plus = minecraftVersion >= MinecraftVersion::V26_1;

	std::vector<TagUPtr> children;

	children.push_back(std::make_unique<TagInt>(50, "WanderingTraderSpawnChance"));
	children.push_back(std::make_unique<TagDouble>(0, "BorderCenterZ"));

	if (is26Plus) {
		std::vector<TagUPtr> difficultyChildren;
		difficultyChildren.push_back(std::make_unique<TagString>("peaceful", "difficulty"));
		difficultyChildren.push_back(std::make_unique<TagByte>(0, "locked"));
		children.push_back(std::make_unique<TagCompound>(std::move(difficultyChildren), "difficulty_settings"));
	} else {
		children.push_back(std::make_unique<TagByte>(0, "Difficulty"));
	}

	children.push_back(std::make_unique<TagLong>(0, "BorderSizeLerpTime"));
	children.push_back(std::make_unique<TagByte>(0, "raining"));
	children.push_back(std::make_unique<TagLong>(0, "Time"));
	children.push_back(std::make_unique<TagInt>(3, "GameType"));
	children.push_back(std::make_unique<TagDouble>(0, "BorderCenterX"));
	children.push_back(std::make_unique<TagDouble>(0.2, "BorderDamagePerBlock"));
	children.push_back(std::make_unique<TagDouble>(5, "BorderWarningBlocks"));

	if (!is26Plus) {
		children.push_back(serialiseWorldGenSettings());
	}

	{
		std::vector<TagUPtr> versionChildren;
		versionChildren.push_back(std::make_unique<TagByte>(0, "Snapshot"));
		versionChildren.push_back(std::make_unique<TagString>("main", "Series"));
		versionChildren.push_back(std::make_unique<TagString>(versionName, "Name"));
		versionChildren.push_back(std::make_unique<TagInt>(dataVersion, "Id"));

		children.push_back(std::make_unique<TagCompound>(std::move(versionChildren), "Version"));
	}

	children.push_back(std::make_unique<TagLong>(6000, "DayTime"));
	children.push_back(std::make_unique<TagByte>(1, "initialized"));
	children.push_back(std::make_unique<TagByte>(1, "allowCommands"));
	children.push_back(std::make_unique<TagInt>(16800, "WanderingTraderSpawnDelay"));
	children.push_back(std::make_unique<TagInt>(dataVersion, "DataVersion"));
	children.push_back(std::make_unique<TagString>(name, "LevelName"));
	children.push_back(std::make_unique<TagByte>(0, "MapFeatures"));
	children.push_back(std::make_unique<TagByte>(0, "thundering"));
	children.push_back(std::make_unique<TagLong>(13, "RandomSeed"));
	children.push_back(std::make_unique<TagInt>(0, "SpawnX"));
	children.push_back(std::make_unique<TagInt>(spawnHeight, "SpawnY"));
	children.push_back(std::make_unique<TagInt>(0, "SpawnZ"));
	children.push_back(std::make_unique<TagInt>(19133, "version"));
	auto now = std::chrono::system_clock::now();
	auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
	children.push_back(std::make_unique<TagLong>(now_ms, "LastPlayed"));
	children.push_back(std::make_unique<TagByte>(0, "WasModded"));

	{
		std::vector<TagUPtr> dataPacksChildren;

		{
			std::vector<TagUPtr> enabledChildren;
			enabledChildren.push_back(std::make_unique<TagString>("vanilla", ""));
			dataPacksChildren.push_back(std::make_unique<TagList>(std::move(enabledChildren), "Enabled", 8));
		}

		dataPacksChildren.push_back(std::make_unique<TagList>(std::vector<TagUPtr>{}, "Disabled", 8));
		children.push_back(std::make_unique<TagCompound>(std::move(dataPacksChildren), "DataPacks"));
	}

	std::vector<TagUPtr> rootChildren;
	rootChildren.push_back(std::make_unique<TagCompound>(std::move(children), "Data"));

	TagCompound root(std::move(rootChildren), "");

	Buffer output(4096);
	root.write(output);
	return Util::Serialisation::gzipCompress(output);
}

Util::Serialisation::NBT::TagCompoundUPtr World::serialiseWorldGenSettings() {
	std::vector<TagUPtr> worldGenChildren;
	worldGenChildren.push_back(std::make_unique<TagByte>(0, "bonus_chest"));
	worldGenChildren.push_back(std::make_unique<TagLong>(13, "seed"));
	worldGenChildren.push_back(std::make_unique<TagByte>(1, "generate_features"));

	{
		std::vector<TagUPtr> dimensionsChildren;

		{
			std::vector<TagUPtr> overworldChildren;
			overworldChildren.push_back(std::make_unique<TagString>("minecraft:overworld", "type"));
			{
				std::vector<TagUPtr> generatorChildren;
				generatorChildren.push_back(std::make_unique<TagString>("minecraft:flat", "type"));

				{
					std::vector<TagUPtr> settingsChildren;
					settingsChildren.push_back(std::make_unique<TagList>(std::vector<TagUPtr>{}, "layers", 10));
					settingsChildren.push_back(std::make_unique<TagList>(std::vector<TagUPtr>{}, "structure_overrides", 8));
					settingsChildren.push_back(std::make_unique<TagString>("minecraft:plains", "biome"));
					settingsChildren.push_back(std::make_unique<TagByte>(0, "features"));
					settingsChildren.push_back(std::make_unique<TagByte>(0, "lakes"));
					generatorChildren.push_back(std::make_unique<TagCompound>(std::move(settingsChildren), "settings"));
				}
				overworldChildren.push_back(std::make_unique<TagCompound>(std::move(generatorChildren), "generator"));
			}

			dimensionsChildren.push_back(std::make_unique<TagCompound>(std::move(overworldChildren), "minecraft:overworld"));
		}

		worldGenChildren.push_back(std::make_unique<TagCompound>(std::move(dimensionsChildren), "dimensions"));
	}

	return std::make_unique<TagCompound>(std::move(worldGenChildren), "WorldGenSettings");
}