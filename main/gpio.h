#pragma once

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include "driver/gpio.h"

int LUA_OPEN_GPIO(lua_State *L);