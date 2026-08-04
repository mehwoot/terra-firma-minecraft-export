
#include "Trees.h"

#include <random>


using namespace Simulation;
using namespace Simulation::Export;
using namespace Simulation::Export::Minecraft;
using namespace Util::Serialisation;

Placeable::Placeable(const std::string& name, const std::list<BlockToConstruct>& blocks) {
	this->name = name;
	this->blocks = blocks;
}

void Placeable::generate(Region& region, const tf_v0_ivec3& position, int randSeed) const {
	for (const auto& block : blocks) {
		auto worldPosition = tf_v0_ivec3{position.x + block.position.x, position.y + block.position.y};
		if (replaceAir) {
			region.setAllBlocksBetweenLocal({worldPosition.x, worldPosition.z}, worldPosition.y, worldPosition.y, block.id);
		} else {
			region.setIfNotAir({worldPosition.x, worldPosition.z}, worldPosition.y, airId, block.id);
		}
	}
}

CollectionPlaceable::CollectionPlaceable(std::string name, std::vector<Placeable> trees) : Placeable(name, {}) {
	this->trees = std::move(trees);
}

void CollectionPlaceable::generate(Region& region, const tf_v0_ivec3& position, int randSeed) const {
	std::default_random_engine rEng{};
	rEng.seed(randSeed);
	auto dist = std::uniform_int_distribution<>{0, static_cast<int>(trees.size())};
	auto index = dist(rEng);
	return trees[index].generate(region, position, randSeed);
}