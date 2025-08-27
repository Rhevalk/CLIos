#pragma once

#include <string.h>
#include <stdlib.h>

#include "lua.h"
#include "lauxlib.h"

#include "esp_http_server.h"

typedef struct {
    httpd_handle_t srv;
    lua_State *L;
} lua_httpd_t;

int LUA_OPEN_HTTPD(lua_State *L);