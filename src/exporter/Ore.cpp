
#include "Ore.h"
#include "mc-export-plugin-module.h"

using namespace Simulation::Export;
using namespace Simulation::Export::Minecraft;
using namespace Util::Serialisation;

OreDefinition OreDefinition::deserialise(Util::Serialisation::NBT::TagCompound& root) {
	OreDefinition value;
	value.name = root.name;

	for (auto& child : root.children) {
		if (child->name == "schematicFolder") {
			value.schematicFolder = child->as<NBT::TagString>().value;
		} else if (child->name == "rockDepthDistribution") {
			value.rockDepthDistribution = Util::Distribution::deserialise(child->as<NBT::TagCompound>());
		} else if (child->name == "worldYPositionDistribution") {
			value.worldYPositionDistribution = Util::Distribution::deserialise(child->as<NBT::TagCompound>());
		} else {
			reportFatalErrorC(std::format("Ore {} unknown attribute: {}", value.name, child->name).c_str());
		}
	}

	if (!value.rockDepthDistribution && !value.worldYPositionDistribution) {
		reportFatalErrorC(std::format("Ore {} missing distribution", value.name).c_str());
	}

	return value;
}

Ore::Ore(int placeableId, std::array<float, 384> rockDepthDistribution, std::array<float, 384> worldYPositionDistribution)
	: placeableId(placeableId),
	  rockDepthDistribution(rockDepthDistribution),
	worldYPositionDistribution(worldYPositionDistribution) {

}

std::optional<int> Ore::shouldPlaceAt(int rockDepth, int worldYPosition, int randSeed) const {
	rockDepth = std::clamp(rockDepth, 0, 383);
	worldYPosition = std::clamp(worldYPosition, 0, 383);
	randomDistribution.seed(randSeed + placeableId);
	float probability = rockDepthDistribution[rockDepth] * worldYPositionDistribution[worldYPosition] * ORE_SPACING;
	auto randValue = randomDistribution.randFloat();
	if (randValue < probability) {
		return placeableId;
	} else {
		return std::nullopt;
	}
}

Ore Ore::create(int placeableId, Util::Distribution* _rockDepthDistribution, Util::Distribution* _worldYPositionDistribution) {
	std::array<float, 384> rockDepthDistribution = {};
	std::array<float, 384> worldYPositionDistribution = {};

	for (int i = 0; i < 384; ++i) {
		rockDepthDistribution[i] = (_rockDepthDistribution ? _rockDepthDistribution->getValue(static_cast<float>(i)) : 1.f);
		worldYPositionDistribution[i] = (_worldYPositionDistribution ? _worldYPositionDistribution->getValue(static_cast<float>(i)) : 1.f);
	}

	return Ore(placeableId, rockDepthDistribution, worldYPositionDistribution);
}