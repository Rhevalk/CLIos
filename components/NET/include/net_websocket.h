#pragma once
#include <string.h>
#include <stdlib.h>

#include "lua.h"
#include "lauxlib.h"

#include "esp_websocket_client.h"

typedef struct {
    esp_websocket_client_handle_t c;
} lua_ws_t;

int LUA_OPEN_WEBSOCKET(lua_State *L);