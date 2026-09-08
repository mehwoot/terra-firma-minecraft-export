#pragma once

#include <Api/v0/Vector.h>

namespace Simulation {
	namespace Export {
		namespace Minecraft {
			struct BlockToConstruct {
				int id;
				tf_v0_ivec3 position;
			};
		}
	}
}