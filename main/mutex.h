#pragma once

#include "lua.h"          
#include "lauxlib.h"      

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

typedef struct {
    SemaphoreHandle_t handle;
} lua_mutex_t;

int LUA_OPEN_MUTEX( lua_State *L );