#pragma once

#include <Api/v0/GameApi.h>

extern tf_v0_GameApi gameApi;

[[noreturn]]
void reportFatalError(char * what);

[[noreturn]]
void reportFatalErrorC(char const* what);

void reportNonFatalError(char * what);
void reportNonFatalErrorC(char const* what);
