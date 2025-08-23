#pragma once

#include "lua.h"
#include "lauxlib.h"
#include "esp_system.h"

#include "driver/uart.h"

#include "global.h"

int LUA_OPEN_OS(lua_State *L);