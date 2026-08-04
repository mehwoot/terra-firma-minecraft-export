
#include <Api/v0/Vector.h>
#include <Api/v0/GameApi.h>

#include <Api/v0/Rasteriser.h>

#include <stdio.h>

tf_v0_GameApi gameApi;


void reportFatalError(char * what){
    gameApi.reportFatalError(gameApi.context, what);
}

void reportFatalErrorC(char const * what){
    gameApi.reportFatalError(gameApi.context, what);
}

void reportNonFatalError(char * what){
    gameApi.reportNonFatalError(gameApi.context, what);
}

void reportNonFatalErrorC(char const * what){
    gameApi.reportNonFatalError(gameApi.context, what);
}

void debugButtonCallback(void* data){
}

void pluginOnLoad(tf_v0_GameApi newApi){
    gameApi = newApi;
    gameApi.log(gameApi.context, "tf2-mc-export pluginOnLoad running");

    gameApi.registerDebugButton(gameApi.context, "tf2-mc-export debugButton", &debugButtonCallback, 0);
}

void pluginUnload(){
    gameApi.log(gameApi.context, "tf2-mc-export pluginUnload running");
}

void exportFunction(tf_v0_ExportDataApi exportDataApi){
    tf_v0_WorldData worldData = exportDataApi.getWorldData(exportDataApi.instance);
    
    printf("tf2-mc-export exportFunction - dimensions: %i x %i\n", worldData.dimensions.x, worldData.dimensions.y);

    // for (int x = 0; x < worldData.dimensions.x; ++x) {
    //     for (int y = 0; y < worldData.dimensions.y; ++y) {
    //         auto const val = worldData.waterArray[(y*worldData.dimensions.x) + x ].waterHeight;
    //         if(val!=0){
    //             printf("    [%i][%i] waterHeight = %i\n", x, y, val);
    //         }
    //     }
    // }

    for (int x = 0; x < worldData.dimensions.x; ++x) {
        for (int y = 0; y < worldData.dimensions.y; ++y) {
            void* instance = worldData.heightCache.instance;
            tf_v0_vec2 pos = {x*1.f,y*1.f};
            float const val = worldData.heightCache.getHeightAt(instance, pos, TF_V0_HM_LAND_AND_WATER);
            if(val!=0){
                printf("    [%i][%i] height@ = %f\n", x, y, val);
            }
        }
    }
}

tf_v0_ExporterCallbacks getExporterCallbacks() {
    tf_v0_ExporterCallbacks toRet = {
        .doExport = &exportFunction,
        .exporterButtonHoverText = "tf2-mc-export",
        .exporterButtonIconPath = "images/mc-export-plugin.png" //TODO with or without 'assets/'?
    };
    return toRet;
};

TF_V0_DEFINE_INITIALISE_PLUGIN_MODULE_FUNC {
    tf_v0_PluginCallbacks thisPlugin = {
        .load = &pluginOnLoad,
        .unload = &pluginUnload,
        .getExporterCallbacks = &getExporterCallbacks,
    };
    pluginsApi.registerPlugin(pluginsApi.context, thisPlugin);
}
