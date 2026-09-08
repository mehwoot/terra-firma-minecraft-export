#pragma once
#include "Tags.h"
#include "Macros.h"

namespace Util {
	StandardTypes(Distribution);
	
	class Distribution {
	public:
		virtual float getValue(float input) const = 0;

		static DistributionUPtr deserialise(Serialisation::NBT::TagCompound& root);

		Distribution() = default;
		
		virtual ~Distribution() = default;
		Distribution(Distribution const&) = default;
		Distribution(Distribution &&) = default;
		Distribution& operator=(Distribution const&) = default;
		Distribution& operator=(Distribution &&) = default;
	};

	class LinearDistribution : public Distribution {
	protected:
		float minStart, minTop, maxTop, maxEnd;
		float maxValue;

	public:
		LinearDistribution(float maxValue, float minStart, float minTop, float maxTop, float maxEnd);
		virtual float getValue(float input) const override;

		static DistributionUPtr deserialise(Serialisation::NBT::TagCompound& root);
	};

}