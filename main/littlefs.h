#pragma once

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include "esp_log.h"
#include "esp_vfs.h"
#include "esp_err.h"
#include "esp_littlefs.h"

void LittleFSInit_(lua_State *L);
int LUA_OPEN_FS(lua_State *L);