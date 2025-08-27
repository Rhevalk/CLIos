#pragma once

#include "lua.h" // Include necessary headers for Lua
#include "lualib.h" // Include necessary headers for Lua libraries
#include "lauxlib.h" // Include necessary headers for Lua auxiliary functions

#include <string.h> 
#include "esp_err.h"

// ADC (v5 driver)
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

typedef struct {
    adc_oneshot_unit_handle_t unit;
    adc_cali_handle_t cali;
    bool cali_enabled;
    adc_unit_t unit_id;
    adc_channel_t channel_id;
    adc_bitwidth_t bitwidth;
} lua_adc_ch_t;

int LUA_OPEN_ADC(lua_State *L);