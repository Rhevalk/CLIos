#pragma once

#include <string.h>
#include <stdlib.h>

#include "lua.h"
#include "lauxlib.h"

#include "esp_http_client.h"

int LUA_OPEN_HTTP(lua_State *L);