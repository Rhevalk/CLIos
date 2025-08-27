#pragma once

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_err.h"

/* NimBLE headers (Apache Mynewt NimBLE as used in ESP-IDF v5) */
#include "esp_nimble_hci.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "host/ble_uuid.h"
#include "esp_bt.h" 
#include "services/gap/ble_svc_gap.h"

/* Lua headers: path depends on your build; ensure include_directories points to Lua */
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

#include "global.h"

/* Lua BLE module init */
int LUA_OPEN_BLE(lua_State *L);
