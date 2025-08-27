#pragma once

#include <string.h>
#include <stdlib.h>

#include "lua.h"
#include "lauxlib.h"

#if __has_include("driver/i2c_master.h")
  #include "driver/i2c_master.h"
  #define HAVE_I2C_V5 1
#else
  #include "driver/i2c.h"
  #define HAVE_I2C_V5 0
#endif

typedef struct {
#if HAVE_I2C_V5
    i2c_master_bus_handle_t bus;
#else
    int port;
#endif
} lua_i2c_bus_t;

typedef struct {
#if HAVE_I2C_V5
    i2c_master_dev_handle_t dev;
#else
    int port; uint8_t addr;
#endif
} lua_i2c_dev_t;

int LUA_OPEN_I2C(lua_State *L);