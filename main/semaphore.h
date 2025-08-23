#pragma once

#include "lua.h"          
#include "lauxlib.h"      

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

typedef struct {
    SemaphoreHandle_t handle;
} lua_semaphore_t;

int LUA_OPEN_SEMAPHORE( lua_State *L );