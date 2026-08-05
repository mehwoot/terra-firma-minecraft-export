#include "McExporterCIFace.h"

#include "McExporter.hpp"

extern "C" {

void doExport(tf_v0_ExportDataApi* api){
	auto exporter = Simulation::Export::Minecraft::McExporter {};
	exporter.run(*api);
}

}