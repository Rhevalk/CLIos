#pragma once

#include <string.h>

#include "lua.h" // Include necessary headers for Lua
#include "lualib.h" // Include necessary headers for Lua libraries
#include "lauxlib.h" // Include necessary headers for Lua auxiliary functions

#include "driver/ledc.h"

typedef struct {
    ledc_timer_config_t tcfg;
    ledc_channel_config_t ccfg;
    bool started;
} lua_pwm_t;

int LUA_OPEN_PWM(lua_State *L);