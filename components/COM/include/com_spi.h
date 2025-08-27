#pragma once

#include <string.h>
#include <stdlib.h>

#include "lua.h"
#include "lauxlib.h"

#include "driver/spi_master.h"

typedef struct {
  spi_device_handle_t dev; 
} lua_spi_dev_t;

int LUA_OPEN_SPI(lua_State *L);