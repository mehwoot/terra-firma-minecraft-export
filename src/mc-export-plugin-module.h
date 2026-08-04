#pragma once

#include <Api/v0/GameApi.h>

tf_v0_GameApi api;

void reportFatalError(char * what);
void reportFatalErrorC(char const* what);
