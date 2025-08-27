#pragma once

#include <string.h>
#include <stdlib.h>

#include "lua.h"
#include "lauxlib.h"

#include "esp_tls.h"

typedef struct {
    esp_tls_t *tls;
} lua_tls_t;


int LUA_OPEN_TLS(lua_State *L);