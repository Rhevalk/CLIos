#pragma once

#include <string.h>
#include <stdlib.h>

#include "lua.h"
#include "lauxlib.h"

#include "mdns.h"

int LUA_OPEN_MDNS(lua_State *L);