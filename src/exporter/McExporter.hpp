#pragma once

#include "Options.h"
#include "Theme.hpp"

#include <Api/v0/ExporterApi.h>

#include <list>
#include <mutex>

namespace Simulation::Export::Minecraft{
class McExporter{
	public:
		struct ExportInstance {
			const tf_v0_Rasteriser& rasteriser;
			const tf_v0_HeightCache& heightCache;
			float worldRatio;
			int regionXStart, regionZStart;
			tf_v0_ivec2 mapDimensions;
			const std::filesystem::path& folder;
			int worldResolution;
			int maxHeight = 383;
			int seaLevel = 16;
			float worldSeaLevel = 0.f;
			BlockRegistry blockRegistry;
			Theme theme;
			MinecraftVersion minecraftVersion = MinecraftVersion::V1_21_1;
		};
		
		class AsyncCoordinator {
		protected:
			struct ExportRegion {
				int x, z;
			};

			struct Output {
				ExportRegion region;
				Buffer buffer;
			};

			const ExportInstance& config;
			std::list<ExportRegion> inputRegions;
			std::list<Output> outputBuffers;
			std::mutex inputLock, outputLock;

		public:
			AsyncCoordinator(const ExportInstance& config);
			void addRegionToExport(int x, int z);
			std::optional<ExportRegion> popInput();
			void pushOutput(Output buffer);
			std::optional<Output> popOutput();
			const ExportInstance& getConfig() const { return config; }
		};

	void run(tf_v0_ExportDataApi& api);

	private:
		static std::list<Simulation::Export::Minecraft::ThemeDefinition> themeDefinitions;
};
}