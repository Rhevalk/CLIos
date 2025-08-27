#pragma once

#include "lua.h" // Include necessary headers for Lua
#include "lualib.h" // Include necessary headers for Lua libraries
#include "lauxlib.h" // Include necessary headers for Lua auxiliary functions

#include <string.h>

// DAC (v5 driver check)
#if __has_include("driver/dac_oneshot.h")
#include "driver/dac_oneshot.h"
#define HAVE_DAC 1
#else
#define HAVE_DAC 0
#endif

typedef struct {
#if HAVE_DAC
    dac_oneshot_handle_t h;
    dac_channel_t ch;
#endif
} lua_dac_t;

// Lua open function
int LUA_OPEN_DAC(lua_State *L);
