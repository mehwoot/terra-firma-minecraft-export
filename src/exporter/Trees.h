#pragma once

// #include "Util/Singleton.h"
#include "Region.h"
#include "BlockToConstruct.h"

namespace Simulation {
	namespace Export {
		namespace Minecraft {
			StandardTypesStruct(Placeable);

			struct Placeable {
				std::string name;
				std::list<BlockToConstruct> blocks;
				bool replaceAir = true;
				int airId = 0;

				Placeable(const std::string& name, const std::list<BlockToConstruct>& blocks);
				virtual void generate(Region& region, const tf_v0_ivec3& position, int randSeed) const;
				void setReplaceAir(bool replaceAir, int airId = 0) {
					this->replaceAir = replaceAir;
					this->airId = airId;
				}
			};

			struct CollectionPlaceable : public Placeable {
				std::vector<Placeable> trees;

				CollectionPlaceable(std::string name, std::vector<Placeable> trees);

				virtual void generate(Region& region, const tf_v0_ivec3& position, int randSeed) const override;
			};
		}
	}
}