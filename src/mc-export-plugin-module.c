

#include "exporter/McExporterCIFace.h"
#include <Api/v0/Vector.h>
#include <Api/v0/GameApi.h>

#include <Api/v0/Rasteriser.h>

tf_v0_GameApi gameApi;

void reportFatalError(char* what) {
	gameApi.reportFatalError(gameApi.context, what);
}

void reportFatalErrorC(char const* what) {
	gameApi.reportFatalError(gameApi.context, what);
}

void reportNonFatalError(char* what) {
	gameApi.reportNonFatalError(gameApi.context, what);
}

void reportNonFatalErrorC(char const* what) {
	gameApi.reportNonFatalError(gameApi.context, what);
}

void debugButtonCallback(void* data) {
}

void pluginOnLoad(tf_v0_GameApi newApi) {
	gameApi = newApi;
	gameApi.log(gameApi.context, "tf2-mc-export pluginOnLoad running");

	gameApi.registerDebugButton(gameApi.context, "tf2-mc-export debugButton", &debugButtonCallback, 0);
}

void pluginUnload() {
	gameApi.log(gameApi.context, "tf2-mc-export pluginUnload running");
}

void exportFunction(tf_v0_ExportDataApi exportDataApi) {
	doExport(&exportDataApi);
}

char** getAvailableThemes() {
	// TODO

	static char* availableThemes[] = { "default" };
	return availableThemes;
}

void makeExporterConfigurationUi(tf_v0_GuiApi guiApi) {
	void* ctx = guiApi.context;

	// map width
	static int exportMapWidthChoices[] = { 1024, 2048, 3072, 4096, 8192, 16384 };
	guiApi.addFixedAlternativesSelectorInt(ctx, "exportMapWidth", exportMapWidthChoices, &exportMapWidthChoices[0]);

	// max height
	guiApi.addBinaryToggle(ctx, "maxHeightSpecified", 0);
	guiApi.addSliderInt(ctx, "maxHeight", 384, 2032, 16, 384);

	// sea level
	guiApi.addArbitraryInputInt(ctx, "seaLevel", 32);

	// Minecraft version
	static char* minecraftVersions[] = {
		"1.20.1",
		"1.20.2 - 1.20.4",
		"1.21.1",
		"26.1+",
	};
	guiApi.addFixedAlternativesSelectorString(ctx, "minecraftVersion", minecraftVersions, &minecraftVersions[3]);

	// theme
	char** availableThemes = getAvailableThemes();
	guiApi.addFixedAlternativesSelectorString(ctx, "theme", availableThemes, &availableThemes[0]);
}

tf_v0_ExporterCallbacks getExporterCallbacks() {
	tf_v0_ExporterCallbacks toRet = {
		&exportFunction,
		&makeExporterConfigurationUi,
		"tf2-mc-export",
		"images/mc-export-plugin.png" // TODO with or without 'assets/'?
	};
	return toRet;
};

TF_V0_DEFINE_INITIALISE_PLUGIN_MODULE_FUNC {
	tf_v0_PluginCallbacks thisPlugin = {
		.pluginFriendlyName = "tf2-mc-export",
		.load = &pluginOnLoad,
		.unload = &pluginUnload,
		.getExporterCallbacks = &getExporterCallbacks,
	};
	pluginsApi.registerPlugin(pluginsApi.context, thisPlugin);
}
