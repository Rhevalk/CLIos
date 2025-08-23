#pragma once

#include "lua.h" // Include necessary headers for Lua
#include "lualib.h" // Include necessary headers for Lua libraries
#include "lauxlib.h" // Include necessary headers for Lua auxiliary functions

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

typedef struct {
    QueueHandle_t handle;
} lua_queue_t;

int LUA_OPEN_QUEUE( lua_State *L );