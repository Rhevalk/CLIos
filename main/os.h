#pragma once

#include "lua.h"
#include "lauxlib.h"
#include "esp_system.h"
#include "esp_pm.h"
#include "esp_sleep.h"
#include "driver/uart.h"

int LUA_OPEN_OS(lua_State *L);