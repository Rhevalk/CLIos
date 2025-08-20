#pragma once

#include "lua.h" // Include necessary headers for Lua
#include "lualib.h" // Include necessary headers for Lua libraries
#include "lauxlib.h" // Include necessary headers for Lua auxiliary functions

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

int LUA_OPEN_TASK( lua_State *L );