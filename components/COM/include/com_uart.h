#pragma once

#include <string.h>
#include <stdlib.h>

#include "lua.h" // Include necessary headers for Lua
#include "lualib.h" // Include necessary headers for Lua libraries
#include "lauxlib.h" // Include necessary headers for Lua auxiliary functions

#include "driver/uart.h"

typedef struct {
    int port; // UART_NUM_0/1/2
    bool installed;
} lua_uart_t;

int LUA_OPEN_UART(lua_State *L);