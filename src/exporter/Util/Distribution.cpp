#include "Distribution.h"

using namespace Util;
using namespace Util::Serialisation;
using namespace Util::Serialisation::NBT;

/* ====================== Distribution ====================== */

DistributionUPtr Distribution::deserialise(Serialisation::NBT::TagCompound& root) {
	auto typeTag = root.findChildByName("type");
	if (!typeTag) {
		throw NBT::ParseError("Distribution missing type");
	}

	std::string& typeName = typeTag->as<NBT::TagString>().value;
	if (typeName == "linear") {
		return LinearDistribution::deserialise(root);
	}

	throw NBT::ParseError(std::format("Unknown distribution type {}", typeName));
}

/* ====================== LinearDistribution ====================== */

LinearDistribution::LinearDistribution(float maxValue, float minStart, float minTop, float maxTop, float maxEnd) 
	: maxValue(maxValue), minStart(minStart), minTop(minTop), maxTop(maxTop), maxEnd(maxEnd) {
	
}

float LinearDistribution::getValue(float input) const {
	if (input < minStart) return 0.f;
	if (input < minTop) return (1.f - ((minTop - input) / (minTop - minStart))) * maxValue;
	if (input < maxTop) return maxValue;
	if (input < maxEnd) return (1.f - ((input - maxTop) / (maxEnd - maxTop))) * maxValue;
	return 0.f;
}

DistributionUPtr LinearDistribution::deserialise(Serialisation::NBT::TagCompound& root) {
	float minStart = 0.f, minTop = 0.f, maxTop = 0.f, maxEnd = 0.f, maxValue = 1.f;

	for (auto& child : root.children) {
		if (child->name == "minStart") {
			minStart = child->as<NBT::TagFloat>().value;
		} else if (child->name == "minTop") {
			minTop = child->as<NBT::TagFloat>().value;
		} else if (child->name == "maxTop") {
			maxTop = child->as<NBT::TagFloat>().value;
		} else if (child->name == "maxEnd") {
			maxEnd = child->as<NBT::TagFloat>().value;
		} else if (child->name == "maxValue") {
			maxValue = child->as<NBT::TagFloat>().value;
		}
	}

	return std::make_unique<LinearDistribution>(maxValue, minStart, minTop, maxTop, maxEnd);
}