#pragma once

#include <string.h>
#include <stdlib.h>

#include "lua.h"
#include "lauxlib.h"

#if __has_include("driver/twai.h")
  #include "driver/twai.h"
  #define HAVE_TWAI 1
#else
  #define HAVE_TWAI 0
#endif

typedef struct {
#if HAVE_TWAI
    bool started;
#endif
} lua_can_t;

int LUA_OPEN_CAN(lua_State *L);