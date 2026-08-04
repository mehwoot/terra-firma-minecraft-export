#include "McExporter.hpp"


#include "Options.h"
#include "Theme.h"
#include "HeightDatapack.h"
#include "World.h"

#include "Util/Perlin.h"
#include "Util/OS.h"
#include "Util/Compression.h"

#include "mc-export-plugin-module.h"

#include <Api/v0/ExporterApi.h>
#include <Api/v0/Rasteriser.h>

#include <filesystem>
#include <format>
#include <list>
#include <iostream>


constexpr int regionSize = 512;
constexpr int chunkSize = 16;
constexpr int TREE_SPACING = 5;
constexpr int REGION_PADDING = 4;
constexpr int totalPointsEachDimension = regionSize + (REGION_PADDING * 2);

using namespace Simulation::Export::Minecraft;

namespace {
    
void createFolderIfDoesntExist(const std::filesystem::path& path) {
	if (!std::filesystem::exists(path)) {
		if (!std::filesystem::create_directories(path)) {
            auto const msg=std::format("could not create directory: {}", path.string());
			reportFatalErrorC(msg.c_str());
		} 
	}
}


std::list<Simulation::Export::Minecraft::ThemeDefinition> themeDefinitions;


float simulationToMinecraftHeight(float simulationHeight, const McExporter::ExportInstance& exportConfig) {
	simulationHeight -= exportConfig.worldSeaLevel;
	if (simulationHeight > 0.f) {
		return std::clamp((simulationHeight * exportConfig.worldRatio) + exportConfig.seaLevel, 0.f, float(exportConfig.maxHeight));
	} else {
		constexpr float oceanCompression = 3.f;
		return std::clamp((simulationHeight * exportConfig.worldRatio / oceanCompression) + exportConfig.seaLevel, 0.f, float(exportConfig.maxHeight));
	}
}

int getMaxHeight(const tf_v0_WorldData& worldData, const McExporter::ExportInstance& exportConfig) {
	float maxSimulationHeight = std::numeric_limits<float>::lowest();
    auto const& size = worldData.size;

	for (int x = 0; x < size.x; x++) {
		for (int z = 0; z < size.y; z++) {
			auto& waterRow0Data = worldData.waterRow0[z*size.x + x];
			maxSimulationHeight = std::max(maxSimulationHeight, waterRow0Data.totalHeight);
		}
	}

	return static_cast<int>(simulationToMinecraftHeight(maxSimulationHeight, exportConfig) + 16);
}

void writeHeightDatapack(const std::filesystem::path& folder, int maxHeight, MinecraftVersion version) {
	std::filesystem::path datapackFolder = folder / "datapacks" / "heights";
	std::filesystem::path dataFolder = datapackFolder / "data" / "minecraft" / "dimension_type";

	createFolderIfDoesntExist(datapackFolder);
	createFolderIfDoesntExist(dataFolder);

	writeMCMeta(datapackFolder, version);
	writeOverworld(dataFolder, maxHeight, version);
}

float getMinecraftTemperature(const tf_v0_WorldData& worldData, const McExporter::ExportInstance& exportConfig) {
	float middleTemp = worldData.worldConfig.getTemperatureCelsius(
        worldData.worldConfig.instance, 0.f, tf_v0_vec2(0.5f, 0.5f), 
        worldData.worldConfig.getGameDaysOfYear(worldData.worldConfig.instance)
    );
	float simulationHeightAtZeroCelsius = middleTemp * 40.f;
	float minecraftHeightAtZeroCelsius = simulationToMinecraftHeight(simulationHeightAtZeroCelsius, exportConfig);
	float minecraftTempAtZero = std::clamp(0.15f + ((minecraftHeightAtZeroCelsius) * 0.00125f), 0.f, 1.f);
	return minecraftTempAtZero;
}

const BiomeDefinition& getBiome(tf_v0_BiomeType biomeType, const McExporter::ExportInstance& exportConfig) {
    switch (biomeType) {
    case TF_V0_BIOME_TROPICAL_RAINFOREST:
        return exportConfig.theme.getBiomeDefinition("jungle");
    case TF_V0_BIOME_TEMPERATE_FOREST:
        return exportConfig.theme.getBiomeDefinition("forest");
    case TF_V0_BIOME_BOREAL_FOREST:
        return exportConfig.theme.getBiomeDefinition("taiga");
    case TF_V0_BIOME_OASIS:
        return exportConfig.theme.getBiomeDefinition("oasis");
    case TF_V0_BIOME_GRASSLANDS:
        return exportConfig.theme.getBiomeDefinition("grasslands");
    case TF_V0_BIOME_SAVANNAH:
        return exportConfig.theme.getBiomeDefinition("savanna");
    case TF_V0_BIOME_MARSH:
        return exportConfig.theme.getBiomeDefinition("marsh");
    case TF_V0_BIOME_SWAMP:
        return exportConfig.theme.getBiomeDefinition("swamp");
	case TF_V0_BIOME_MANGROVE_SWAMP:
		return exportConfig.theme.getBiomeDefinition("mangrove_swamp");
	case TF_V0_BIOME_HOT_DESERT:
		return exportConfig.theme.getBiomeDefinition("hot_desert");
    case TF_V0_BIOME_NOTHING:
    default:
        return exportConfig.theme.getBiomeDefinition("nothing");
    }
}

void placeWaterColumn(Region& region, tf_v0_ivec2 coords, int yFrom, float yToFloat, const BlockRegistry& blockRegistry) {
    if (yToFloat < static_cast<float>(yFrom)) return;

    int yTopBlock = static_cast<int>(std::floor(yToFloat));
    float fraction = yToFloat - static_cast<float>(yTopBlock);

    int sourceId = blockRegistry.getId({ "minecraft:water" });

    region.setAllBlocksBetweenLocal(coords, yFrom, yTopBlock, sourceId);

    if (fraction < 1.f/16.f) return;

    int topY = yTopBlock + 1;
    if (topY > region.maxHeight) return;

    int level = std::clamp(static_cast<int>(std::round((1.f - fraction) * 8.f)), 0, 7);
    if (level == 0) {
        region.setAllBlocksBetweenLocal(coords, topY, topY, sourceId);
    } else {
        Block flowingWater;
        flowingWater.name = "minecraft:water";
        flowingWater.properties["level"] = std::to_string(level);
        region.setAllBlocksBetweenLocal(coords, topY, topY, blockRegistry.getId(flowingWater));
    }
}

std::pair<int, int> getCaveSpan(tf_v0_ivec2 worldPosition, int rockMaxHeight) {
	float height = Util::perlinNoise01(worldPosition.x / 64.f, worldPosition.y / 64.f, 17);
	tf_v0_ivec2 offset = tf_v0_ivec2(
		static_cast<int>(Util::perlinNoise2(worldPosition.x / 64.f, worldPosition.y / 64.f, 31) * 128.f),
		static_cast<int>(Util::perlinNoise2(worldPosition.x / 64.f, worldPosition.y / 64.f, 37) * 128.f)
	);
	worldPosition = tf_v0_ivec2{worldPosition.x + offset.x, worldPosition.y + offset.y};

	float xThreshold = Util::perlinNoise01(worldPosition.x / 256.f, worldPosition.y / 48.f, 19);
	float xSizePerlin = Util::perlinNoise01(worldPosition.x / 32.f, worldPosition.y / 32.f, 23);
	xSizePerlin = xSizePerlin * xSizePerlin * xSizePerlin;
	float xSize = std::min(rockMaxHeight, 384) * 0.2f * xSizePerlin;
	if (xThreshold > 0.8f) {
		float middle = height * rockMaxHeight;
		return { std::clamp(static_cast<int>(middle - xSize), 1, rockMaxHeight - 2), std::clamp(static_cast<int>(middle + xSize), 1, rockMaxHeight - 2) };
	}

	float yThreshold = Util::perlinNoise01(worldPosition.x / 48.f, worldPosition.y / 256.f, 454);
	float ySizePerlin = Util::perlinNoise01(worldPosition.x / 32.f, worldPosition.y / 32.f, 777);
	ySizePerlin = ySizePerlin * ySizePerlin * ySizePerlin;
	float ySize = std::min(rockMaxHeight, 384) * 0.2f * ySizePerlin;
	if (yThreshold > 0.8f) {
		float middle = height * rockMaxHeight;
		return { std::clamp(static_cast<int>(middle - ySize), 1, rockMaxHeight - 2), std::clamp(static_cast<int>(middle + ySize), 1, rockMaxHeight - 2) };
	}

	return { -1, -1 };
}

void fixWater(Region& region, const std::vector<float>& waterHeights) {
    for (int x = 0; x < regionSize; x++) {
        for (int z = 0; z < regionSize; z++) {
            int _x = x + REGION_PADDING, _z = z + REGION_PADDING;
            int index = (_x * totalPointsEachDimension) + _z;
            if (waterHeights[index] >= 0.f) {
                float maxNeighbour = -1.f;
                maxNeighbour = std::max(waterHeights[(_x - 1) * totalPointsEachDimension + _z], maxNeighbour);
                maxNeighbour = std::max(waterHeights[(_x + 1) * totalPointsEachDimension + _z], maxNeighbour);
                maxNeighbour = std::max(waterHeights[_x * totalPointsEachDimension + _z - 1], maxNeighbour);
                maxNeighbour = std::max(waterHeights[_x * totalPointsEachDimension + _z + 1], maxNeighbour);

                if (maxNeighbour > waterHeights[index]) {
                    int oldTopBlock = static_cast<int>(std::floor(waterHeights[index]));
                    placeWaterColumn(region, tf_v0_ivec2(x, z), oldTopBlock, maxNeighbour, region.blockRegistry);
                }
            }
        }
    }
}

Buffer exportRegion(int regionX, int regionZ, const McExporter::ExportInstance& exportConfig) {
	int dataVersion = getDataVersion(exportConfig.minecraftVersion);
    Region region(exportConfig.blockRegistry, tf_v0_ivec2(regionX, regionZ), exportConfig.maxHeight, dataVersion);
	const BlockRegistry& blockRegistry = exportConfig.blockRegistry;
    RegionBiomes regionBiomes;
    region.setWorldRatio(exportConfig.worldRatio, exportConfig.mapDimensions);
    auto const regionStartGameCoords = tf_v0_vec2((regionX - exportConfig.regionXStart) * regionSize / exportConfig.worldRatio, (regionZ - exportConfig.regionZStart) * regionSize / exportConfig.worldRatio);
    auto const regionStartMinecraftCoords = tf_v0_ivec2(regionX * regionSize, regionZ * regionSize);
	std::list<Placement> shrubs, trees, ores;
	std::list<tf_v0_ivec2> snowPoints;
    tf_v0_ivec2 regionLocalPosition((regionX - exportConfig.regionXStart) * regionSize, (regionZ - exportConfig.regionZStart) * regionSize);

	std::list<tf_v0_ivec2> debugPoints = {};/* {
        {-329, 82}, {-329, 83}, {-329, 84},
        {-328, 83}, {-330, 83},
    };*/

    std::vector<float> waterHeights(totalPointsEachDimension * totalPointsEachDimension, -1.f);

	Util::DeterministicRandom randomDistribution;

    for (int localX = -REGION_PADDING; localX < regionSize + REGION_PADDING; localX++) {
        for (int localZ = -REGION_PADDING; localZ < regionSize + REGION_PADDING; localZ++) {
            auto worldPosition = tf_v0_vec2{regionStartGameCoords.x + localX/exportConfig.worldRatio, regionStartGameCoords.y + localZ/exportConfig.worldRatio};
            auto const localPosition = tf_v0_ivec2{regionLocalPosition.x + localX, regionLocalPosition.y+localZ};
			auto minecraftRegionCoords= tf_v0_ivec2{localX, localZ};
            auto const minecraftWorldCoords = tf_v0_ivec2{regionStartMinecraftCoords.x + localX, regionStartMinecraftCoords.y+ localZ};

            if (!exportConfig.rasteriser.validPoint(exportConfig.rasteriser.instance, localPosition)) continue;
            auto worldPoint = exportConfig.rasteriser.getWorldPoint(exportConfig.rasteriser.instance, localPosition);
            // tf_v0_ivec2 worldPosInt = {static_cast<int>(worldPosition.x), static_cast<int>(worldPosition.y)};
            // tf_v0_ivec2 dataPosition = tf_v0_convertWorldToLocalInt(worldPosInt, exportConfig.worldResolution);

            float landHeight = simulationToMinecraftHeight(worldPoint.landHeight, exportConfig);
            int waterFrom = -1;
            float waterTo = -1.f;

#if 0
            //note: not ported yet
            if (std::find(debugPoints.begin(), debugPoints.end(), minecraftWorldCoords) != debugPoints.end()) {
                std::cout << std::endl;
                std::cout << std::format("Point ({}, {})", minecraftWorldCoords.x, minecraftWorldCoords.y) << std::endl;
                std::cout << std::format("sim height: {}", worldPoint.landHeight) << std::endl;
				std::cout << std::format("mc height: {}", landHeight) << std::endl;
                exportConfig.rasteriser.debugPoint(localPosition);
            }
#endif

            if (worldPoint.waterHeight > 0.f && landHeight < exportConfig.maxHeight) {
                waterFrom = landHeight;
                waterTo = std::clamp(simulationToMinecraftHeight(worldPoint.landHeight + worldPoint.waterHeight, exportConfig), landHeight, float(exportConfig.maxHeight));
            }

            int grassFrom = -1, grassTo = -1;
            waterFrom = std::clamp(waterFrom, -1, exportConfig.maxHeight);
            waterTo = std::clamp(waterTo, -1.f, static_cast<float>(exportConfig.maxHeight));
			int iceFrom = -1, iceTo = -1;
			if (worldPoint.iceDepth >= 1.f) {
                iceFrom = std::clamp(static_cast<int>(landHeight) + 1, -1, exportConfig.maxHeight);
                iceTo = std::clamp(static_cast<int>(landHeight + (worldPoint.iceDepth * exportConfig.worldRatio)), -1, exportConfig.maxHeight);
				iceTo = std::max(iceTo, iceFrom);
			} 
			if (worldPoint.inSnow) {
				snowPoints.push_back(minecraftRegionCoords);
			}
			int lavaFrom = -1, lavaTo = -1;
			if (worldPoint.lavaHeight > 0.f) {
				lavaFrom = std::clamp(static_cast<int>(landHeight) + 1, -1, exportConfig.maxHeight);
				lavaTo = std::clamp(static_cast<int>(landHeight + (worldPoint.lavaHeight * exportConfig.worldRatio)), -1, exportConfig.maxHeight);
				lavaTo = std::max(lavaTo, lavaFrom);
			}

            float worldHeightAt = worldPoint.landHeight;
            int currentMinecraftPosition = landHeight;
            float grassCoverage = std::max(worldPoint.grassCoverage, worldPoint.marshCoverage);
            if (worldPoint.biomes[0].biomeType == TF_V0_BIOME_BOREAL_FOREST || worldPoint.biomes[0].biomeType == TF_V0_BIOME_TEMPERATE_FOREST ||
                worldPoint.biomes[0].biomeType == TF_V0_BIOME_TEMPERATE_RAINFOREST || worldPoint.biomes[0].biomeType == TF_V0_BIOME_TROPICAL_RAINFOREST) {
                grassCoverage = 1.f;
            }

			int randSeed = (minecraftWorldCoords.x * 117) + (minecraftWorldCoords.y * 299);
			randomDistribution.seed(randSeed);
			float biomeRand = randomDistribution.randFloat();
			tf_v0_BiomeType biomeType;
			float biomeProbability = 0.f;
			for (auto& biomeEntry : worldPoint.biomes) {
				if (biomeRand < biomeProbability + biomeEntry.coverage) {
					biomeType = biomeEntry.biomeType;
					break;
				}
				biomeProbability += biomeEntry.coverage;
			}
            const BiomeDefinition& biome = getBiome(biomeType, exportConfig);

            if (localX % 16 == 0 && localZ % 16 == 0) {
                regionBiomes.setBiome(minecraftRegionCoords, { biome, 1.f });
            }

			tf_v0_vec2 worldRandPosition = region.convertMinecraftRegionCoordinatesToGameWorldCoordinates(tf_v0_convertIVec2ToVec2(minecraftRegionCoords));

			// shrubs
			if (!worldPoint.inSnow) {
				float shrubRand = randomDistribution.randFloat();
				for (auto& plantDefinition : biome.shrubs) {
					if (shrubRand < plantDefinition.probabilityLimit) {
						if (worldPoint.waterHeight <= 0.f) {
							shrubs.emplace_back(
								plantDefinition.id,
								tf_v0_ivec3(minecraftRegionCoords.x, static_cast<int>(simulationToMinecraftHeight(worldPoint.landHeight, exportConfig)) + 1, minecraftRegionCoords.y),
								randSeed
							);
						}
						break;
					}
				}
			}

			// trees
            if (minecraftWorldCoords.x % TREE_SPACING == 0 && minecraftWorldCoords.y % TREE_SPACING == 0) {
				int randSeed = (minecraftWorldCoords.x * 257) + (minecraftWorldCoords.y * 307);
				randomDistribution.seed(randSeed);
				tf_v0_vec2 minecraftPosition = tf_v0_vec2(minecraftRegionCoords.x + randomDistribution.randInt(-1, 2), minecraftRegionCoords.y + randomDistribution.randInt(-1, 2));
				tf_v0_vec2 worldPosition = region.convertMinecraftRegionCoordinatesToGameWorldCoordinates(minecraftPosition);
				float gameLandHeight = exportConfig.heightCache.getHeightAt(exportConfig.heightCache.instance, worldPosition, TF_V0_HM_LAND_ONLY);
				float gameWaterHeight = exportConfig.heightCache.getHeightAt(exportConfig.heightCache.instance, worldPosition, TF_V0_HM_WATER_ONLY);

                float treeRand = randomDistribution.randFloat();
                for (auto& plantDefinition : biome.trees) {
                    if (treeRand < plantDefinition.probabilityLimit) {
                        if (gameWaterHeight <= 0.f) {
                            trees.emplace_back(
								plantDefinition.id,
                                tf_v0_ivec3(minecraftPosition.x, simulationToMinecraftHeight(gameLandHeight, exportConfig) + 1, minecraftPosition.y),
								randSeed
                            );
                        }
                        break;
                    }
                }
            }

            if (localX < 0 || localZ < 0 || localX >= regionSize || localZ >= regionSize) continue;
            bool hasSand = worldPoint.dirtCoverage <= 0.5f;
            bool hasDirt = false;
            bool hasGrass = false;
			int maxRockLayer = -1;

            for (auto& rockLayer : worldPoint.rockLayers) {
                float bottomHeight = simulationToMinecraftHeight(rockLayer.bottomLimit, exportConfig);
                int bottomMinecraftPosition = std::min(currentMinecraftPosition, static_cast<int>(bottomHeight));
                if (worldHeightAt < rockLayer.bottomLimit) {
                    continue;
                }
                worldHeightAt -= rockLayer.depth;

                Block minecraftBlock;
                switch (rockLayer.rockType) {
                case TF_V0_ROCK_DIRT:
					minecraftBlock = hasSand ? Block{ "minecraft:sand" } : Block{ "minecraft:dirt" };
                    break;
                case TF_V0_ROCK_IGNEOUS:
					minecraftBlock = Block{ "minecraft:deepslate" };
                    break;
                case TF_V0_ROCK_SEDIMENTARY:
					minecraftBlock = Block{ "minecraft:sandstone" };
                    break;
                default:
					minecraftBlock = Block{ "minecraft:dirt" };
                    break;
                }
                if (rockLayer.rockType == TF_V0_ROCK_DIRT) {
                    if (Util::randFloat() > rockLayer.depth) continue;
                    hasDirt = true;
                    if (Util::randFloat() < grassCoverage && !hasSand) {
                        hasGrass = true;
                    }
				} else {
					maxRockLayer = std::max(maxRockLayer, currentMinecraftPosition);
				}
                region.setAllBlocksBetweenLocal(minecraftRegionCoords, bottomMinecraftPosition, currentMinecraftPosition, blockRegistry.getId(minecraftBlock));
                currentMinecraftPosition = bottomMinecraftPosition - 1;
            }

            if (hasDirt && hasGrass) {
                grassFrom = landHeight;
                grassTo = landHeight;
            }
            if (grassFrom != -1 && grassTo != -1) {
                region.setAllBlocksBetweenLocal(minecraftRegionCoords, grassFrom, grassTo, blockRegistry.getId({ "minecraft:grass_block" }));
            }
            if (waterFrom != -1 && waterTo >= 0.f) {
                placeWaterColumn(region, minecraftRegionCoords, waterFrom, waterTo, blockRegistry);
                waterHeights[((localX + REGION_PADDING) * totalPointsEachDimension) + (localZ + REGION_PADDING)] = waterTo;
            }
            if (iceFrom != -1 && iceTo != -1) {
                region.setAllBlocksBetweenLocal(minecraftRegionCoords, iceFrom, iceTo, blockRegistry.getId({ "minecraft:snow_block" }));
            }
            if (lavaFrom != -1 && lavaTo != -1) {
                region.setAllBlocksBetweenLocal(minecraftRegionCoords, lavaFrom, lavaTo, blockRegistry.getId({ "minecraft:lava" }));
            }
            region.setAllBlocksBetweenLocal(minecraftRegionCoords, 0, 0, blockRegistry.getId({ "minecraft:bedrock" }));

			for (int y = 0; y <= maxRockLayer; y += ORE_SPACING) {
				int randSeed = (minecraftWorldCoords.x * 939193) + (minecraftWorldCoords.y * 27361) + (y * 115249);
				randomDistribution.seed(randSeed + 289);
				int actualY = y + (randomDistribution.randInt() % ORE_SPACING);

				if (auto placeId = exportConfig.theme.shouldPlaceOreAt(maxRockLayer - actualY, actualY, randSeed)) {
					ores.emplace_back(
						*placeId,
						tf_v0_ivec3(minecraftRegionCoords.x, actualY, minecraftRegionCoords.y),
						randSeed
					);
				}
			}

			auto caveSpan = getCaveSpan(minecraftWorldCoords, maxRockLayer);
			if (caveSpan.first != -1 && caveSpan.second != -1) {
				region.setAllBlocksBetweenLocal(minecraftRegionCoords, caveSpan.first, caveSpan.second, blockRegistry.getId({ "minecraft:air" }));
			}
        }
    }

    fixWater(region, waterHeights);

    for (auto& plant : shrubs) {
		exportConfig.theme.generate(region, plant);
    }
	// place trees last so they can overwrite other plants
    for (auto& plant : trees) {
		exportConfig.theme.generate(region, plant);
    }

	for (auto& ore : ores) {
		exportConfig.theme.generate(region, ore);
	}

	for (auto& snowPoint : snowPoints) {
		region.setTopBlocksTo(snowPoint, blockRegistry.getId({ "minecraft:air" }), blockRegistry.getId({ "minecraft:snow" }));
	}

    for (int chunkX = 0; chunkX < 32; chunkX++) {
        for (int chunkZ = 0; chunkZ < 32; chunkZ++) {
            auto& biomeHere = regionBiomes.getBiome(tf_v0_ivec2(chunkX*16, chunkZ*16));
            region.chunks[(chunkZ * 32) + chunkX].setBiome(biomeHere.definition.minecraftResourceLocation);
        }
    }

    return region.write();
}


}

void McExporter::run(tf_v0_ExportDataApi& api) {

	// const auto& options = *reinterpret_cast<Options*>(api.getOptions(api.instance));
	const auto& options = *reinterpret_cast<Options*>(tf_v0_getOptions(api));

    auto&& worldData = api.getWorldData(api.instance);

    auto const worldSize = worldData.size;
    auto const worldResolution = worldData.resolution;
	tf_v0_ivec2 exportDimensions;
	float zRatio = (static_cast<float>(worldSize.y)) / worldSize.x;
	if (worldSize.y > worldSize.x) {
		exportDimensions = tf_v0_ivec2{options.exportMapWidth, static_cast<int>(options.exportMapWidth * zRatio)};
	} else {
		exportDimensions = tf_v0_ivec2{static_cast<int>(options.exportMapWidth / zRatio), options.exportMapWidth};
	}


    auto const countRegions = tf_v0_ivec2{exportDimensions.x / regionSize, exportDimensions.y / regionSize};
    std::filesystem::path rootFolder = std::filesystem::path(options.folder) / std::filesystem::path(options.filename);
    std::filesystem::path regionFolder;
	std::filesystem::path dataFolder = rootFolder / "data" / "minecraft";
    if (options.minecraftVersion >= MinecraftVersion::V26_1) {
        regionFolder = rootFolder / "dimensions" / "minecraft" / "overworld" / "region";
    } else {
        regionFolder = rootFolder / "region";
    }
    createFolderIfDoesntExist(rootFolder);
    createFolderIfDoesntExist(regionFolder);
    createFolderIfDoesntExist(dataFolder);

	auto rasteriser =  gameApi.getNewRasteriser(gameApi.context, &worldData, exportDimensions);
    float worldRatio = static_cast<float>(exportDimensions.x) / worldSize.x;
    auto const exportSize = tf_v0_vec2{worldSize.x * worldRatio, worldSize.y*worldRatio};

	api.setProgress(api.instance, 0.1f);

	int regionXStart = -countRegions.x / 2, regionXEnd = (countRegions.x / 2) - 1,
		regionZStart = -countRegions.y / 2, regionZEnd = std::max((countRegions.y / 2) - 1, 0);

	auto themeDefinition = std::find_if(themeDefinitions.begin(), themeDefinitions.end(), [&options](const Simulation::Export::Minecraft::ThemeDefinition& theme) {
		return theme.id == options.themeId;
	});
	if (themeDefinition == themeDefinitions.end()) {
		reportFatalErrorC(std::format("Theme {} not found", options.themeId).c_str());
	}

	std::list<tf_v0_ivec2> worldDebugPoints = {};/* {
		{173, 276}, {174, 276}, {175, 276},
		{173, 277}, {174, 277}, {175, 277},
		{173, 278}, {174, 278}, {175, 278},
	};*/

	for (auto& worldDebugPoint : worldDebugPoints) {
		auto& waterData = worldData.waterRow0[(worldDebugPoint.y * worldSize.x) + worldDebugPoint.x];
		auto& waterOffsetData = worldData.waterOffsetArray[(worldDebugPoint.y * worldSize.x) + worldDebugPoint.x];
		std::cout << std::format("Debug Point ({}, {}): LandHeight={} WaterHeight={}, Offset=({}, {})",
			worldDebugPoint.x, worldDebugPoint.y, waterData.landHeight, waterData.waterHeight, waterOffsetData.offsetX, waterOffsetData.offsetY) << std::endl;
	}

    ExportInstance exportConfig{
        .rasteriser = rasteriser,
        .heightCache = worldData.heightCache,
        .worldRatio = worldRatio,
        .regionXStart = regionXStart,
        .regionZStart = regionZStart,
		.mapDimensions = exportDimensions,
        .folder = regionFolder,
        .worldResolution = worldData.resolution,
		.maxHeight = 2031,
		.seaLevel = options.seaLevel,
        .worldSeaLevel = worldData.worldConfig.worldMetresToSimulationUnits(worldData.instance, worldData.worldConfig.seaLevelMetres),
		.blockRegistry = {},
		.theme = themeDefinition->loadTheme(),
		.minecraftVersion = options.minecraftVersion
    };
	exportConfig.theme.copyDatapackFiles(rootFolder);
	exportConfig.theme.loadGenerators(exportConfig.blockRegistry);

	if (options.maxHeight) {
		exportConfig.maxHeight = *options.maxHeight;
	} else {
		exportConfig.maxHeight = getMaxHeight(worldData, exportConfig);
	}
	exportConfig.maxHeight = std::clamp(exportConfig.maxHeight, 383, 2031);
	exportConfig.maxHeight = (((exportConfig.maxHeight / 16) + 1) * 16) - 1; // round up to nearest chunk, minus 1 block

	// write custom datapack for height if we need it
	if (exportConfig.maxHeight > 383) {
		writeHeightDatapack(rootFolder, exportConfig.maxHeight + 1, options.minecraftVersion);
	}
	float baseTemperature = getMinecraftTemperature(worldData, exportConfig);
	writeBiomeDatapack(rootFolder, baseTemperature, options.minecraftVersion);

    AsyncCoordinator asyncCoordinator(exportConfig);
    int regionsToProcess = 0;

    for (int regionX = regionXStart; regionX <= regionXEnd; regionX++) {
        for (int regionZ = regionZStart; regionZ <= regionZEnd; regionZ++) {
            asyncCoordinator.addRegionToExport(regionX, regionZ);
            regionsToProcess++;
        }
    }

    if (api.getCancelled(api.instance)) {
        return;
    }

    auto processor = [&asyncCoordinator]() {
        bool processedRegion = true;
        while (processedRegion) {
            auto regionOpt = asyncCoordinator.popInput();
            if (regionOpt.has_value()) {
                auto& region = regionOpt.value();
                asyncCoordinator.pushOutput(
                    {
                        region,
                        exportRegion(region.x, region.z, asyncCoordinator.getConfig())
                    }
                );
            } else {
                processedRegion = false;
            }
        }
    };

    std::vector<std::thread> threads;
    
    for (int i = 0; i < Util::OS::getCpuCores(); i++) {
        threads.emplace_back(processor);
    }

    int regionsProcessed = 0;
    while (regionsProcessed < regionsToProcess) {
        auto outputOpt = asyncCoordinator.popOutput();
        if (outputOpt) {
            regionsProcessed++;
            api.setProgress(api.instance, 0.1f + (regionsProcessed / static_cast<float>(regionsToProcess) * 0.88f));
            auto& output = outputOpt.value();
            output.buffer.writeToFile(exportConfig.folder / std::filesystem::path(std::format("r.{}.{}.mca", output.region.x, output.region.z)));
        }
        if (api.getCancelled(api.instance)) {
            while (regionsProcessed < regionsToProcess) {
                asyncCoordinator.popInput();
                regionsProcessed++;
            }
        }
    }

    for (auto& thread : threads) {
        thread.join();
    }

    if (api.getCancelled(api.instance)) {
        return;
    }

    tf_v0_vec2 middleCoordinates = tf_v0_vec2(worldSize.x / 2, worldSize.y/2);
    float spawnHeight = worldData.heightCache.getHeightAt(worldData.heightCache.instance, middleCoordinates, TF_V0_HM_LAND_AND_WATER) + 20.f;

    Minecraft::World minecraftWorld(options.filename, spawnHeight, options.minecraftVersion);
    Buffer minecraftWorldBuffer = minecraftWorld.write();
    minecraftWorldBuffer.writeToFile(rootFolder / "level.dat");
    using namespace Util::Serialisation::NBT;
	if (options.minecraftVersion >= MinecraftVersion::V26_1) {
        {
            auto data = minecraftWorld.serialiseWorldGenSettings();
            data->name = "data";
            std::vector<TagUPtr> children;
            children.push_back(std::move(data));
            children.push_back(std::make_unique<TagInt>(getDataVersion(options.minecraftVersion), "DataVersion"));
            TagCompound root(std::move(children), "");

            Buffer output(8192);
            root.write(output);
            Util::Serialisation::gzipCompress(output).writeToFile(dataFolder / "world_gen_settings.dat");

        }

        {
            std::vector<TagUPtr> clockChildren;
            clockChildren.push_back(std::make_unique<TagLong>(6000, "minecraft:overworld"));
            clockChildren.push_back(std::make_unique<TagLong>(0, "minecraft:the_end"));
            auto clockData = std::make_unique<TagCompound>(std::move(clockChildren), "data");

            std::vector<TagUPtr> clockRootChildren;
            clockRootChildren.push_back(std::move(clockData));
            clockRootChildren.push_back(std::make_unique<TagInt>(getDataVersion(options.minecraftVersion), "DataVersion"));
            TagCompound clockRoot(std::move(clockRootChildren), "");

            Buffer clockOutput(8192);
            clockRoot.write(clockOutput);
            Util::Serialisation::gzipCompress(clockOutput).writeToFile(dataFolder / "world_clocks.dat");
        }
    }
}
