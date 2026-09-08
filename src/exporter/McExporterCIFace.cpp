#include "McExporterCIFace.h"

#include "McExporter.hpp"

extern "C" {

tf_v0_ExportResult doExport(tf_v0_ExportDataApi* api) {
	auto exporter = Simulation::Export::Minecraft::McExporter {};
	return exporter.run(*api);
}
}