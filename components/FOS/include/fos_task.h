#pragma once

#include "lua.h" // Include necessary headers for Lua
#include "lualib.h" // Include necessary headers for Lua libraries
#include "lauxlib.h" // Include necessary headers for Lua auxiliary functions

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "global.h"

typedef struct {
    lua_State *L;
    int func_ref;
    TaskHandle_t task_handle;
    QueueHandle_t queue;
    bool stop_flag;
    int timer;
} lua_task_param_t;

int LUA_OPEN_TASK( lua_State *L );