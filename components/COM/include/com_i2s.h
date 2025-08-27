#pragma once

#include <string.h>
#include <stdlib.h>

#include "lua.h"
#include "lauxlib.h"

#include "esp_log.h"
#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#if __has_include("driver/i2s_std.h")
  #include "driver/i2s_std.h"
  #include "driver/i2s_common.h"
  #define HAVE_I2S_V5 1
#else
  #include "driver/i2s.h"
  #define HAVE_I2S_V5 0
#endif

typedef struct {
#if HAVE_I2S_V5
    i2s_chan_handle_t tx; i2s_chan_handle_t rx;
#else
    int port; bool installed;
#endif
} lua_i2s_t;

int LUA_OPEN_I2S(lua_State *L);