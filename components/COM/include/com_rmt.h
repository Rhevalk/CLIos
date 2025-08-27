#pragma once

#include <string.h>
#include <stdlib.h>

#include "lua.h"
#include "lauxlib.h"

#include "freertos/FreeRTOS.h"


#if __has_include("driver/rmt_tx.h")
  #include "driver/rmt_tx.h"
  #include "driver/rmt_rx.h"
  #include "driver/rmt_encoder.h"
  #define HAVE_RMT_V5 1
#else
  #include "driver/rmt.h"
  #define HAVE_RMT_V5 0
#endif

typedef struct {
#if HAVE_RMT_V5
    rmt_channel_handle_t ch; rmt_encoder_handle_t enc;
#else
    rmt_channel_t ch; bool installed;
#endif
} lua_rmt_t;

int LUA_OPEN_RMT(lua_State *L);