// ============================= IMPLEMENTATION =============================
// ch = adc.channel(unit, channel, atten_db?, want_cali?)
//
// ch = adc.channel(adc.UNIT_1, 0, 11, true)
// 
// for i = 1, 5 do
//     local raw = ch:read_raw()
//     local mv, err = ch:read_mv()
//     if mv then
//         print(string.format("Raw=%d, Voltage=%d mV", raw, mv))
//     else
//         print("Raw=", raw, " Err=", err)
//     end
//     tmr.delay(500000) -- 500ms
// end
// 
// ch:close()
// ============================= IMPLEMENTATION =============================

#include "hdw_adc.h"

// Forward declarations
static int l_adc_read_raw(lua_State *L);
static int l_adc_read_mv(lua_State *L);
static int l_adc_close(lua_State *L);

static int l_adc_channel_gc(lua_State *L) {
    lua_adc_ch_t *ch = (lua_adc_ch_t*)lua_touserdata(L, 1);
    if (!ch) return 0;
    if (ch->unit) {
        adc_oneshot_del_unit(ch->unit);
        ch->unit = NULL;
    }
    if (ch->cali) {
        adc_cali_delete_scheme_line_fitting(ch->cali);
        ch->cali = NULL;
    }
    return 0;
}

static int l_adc_channel(lua_State *L) {
    int unit = luaL_checkinteger(L, 1); // 1 or 2
    int channel = luaL_checkinteger(L, 2);
    int atten_db = luaL_optinteger(L, 3, 11);
    int want_cali = lua_toboolean(L, 4);

    lua_adc_ch_t *ch = (lua_adc_ch_t*)lua_newuserdata(L, sizeof(*ch));
    memset(ch, 0, sizeof(*ch));

    ch->unit_id = (unit==1)?ADC_UNIT_1:ADC_UNIT_2;
    ch->bitwidth = ADC_BITWIDTH_DEFAULT;

    adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id = ch->unit_id,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit_cfg, &ch->unit));

    ch->channel_id = (adc_channel_t)channel;
    adc_oneshot_chan_cfg_t chan_cfg = {
        .bitwidth = ch->bitwidth,
        .atten = (atten_db >= 12)?ADC_ATTEN_DB_12:
                 (atten_db >= 6)?ADC_ATTEN_DB_6:
                 (atten_db >= 3)?ADC_ATTEN_DB_2_5:
                                  ADC_ATTEN_DB_0,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(ch->unit, ch->channel_id, &chan_cfg));

    ch->cali_enabled = false;
#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    if (want_cali) {
        adc_cali_line_fitting_config_t cali_cfg = {
            .unit_id = ch->unit_id,
            .atten = chan_cfg.atten,
            .bitwidth = ch->bitwidth,
        };
        if (adc_cali_create_scheme_line_fitting(&cali_cfg, &ch->cali) == ESP_OK) {
            ch->cali_enabled = true;
        }
    }
#endif

    if (luaL_newmetatable(L, "hw.adc.ch")) {
        lua_pushcfunction(L, l_adc_channel_gc); lua_setfield(L, -2, "__gc");

        lua_newtable(L);
        lua_pushcfunction(L, l_adc_read_raw); lua_setfield(L, -2, "read_raw");
        lua_pushcfunction(L, l_adc_read_mv); lua_setfield(L, -2, "read_mv");
        lua_pushcfunction(L, l_adc_close); lua_setfield(L, -2, "close");

        lua_setfield(L, -2, "__index");
    }
    lua_setmetatable(L, -2);
    return 1;
}

// Implementasi metode metatable
static int l_adc_read_raw(lua_State *L) {
    lua_adc_ch_t *ch = (lua_adc_ch_t*)luaL_checkudata(L, 1, "hw.adc.ch");
    int raw = 0;
    ESP_ERROR_CHECK(adc_oneshot_read(ch->unit, ch->channel_id, &raw));
    lua_pushinteger(L, raw);
    return 1;
}

static int l_adc_read_mv(lua_State *L) {
    lua_adc_ch_t *ch = (lua_adc_ch_t*)luaL_checkudata(L, 1, "hw.adc.ch");
    int raw = 0;
    ESP_ERROR_CHECK(adc_oneshot_read(ch->unit, ch->channel_id, &raw));

    if (ch->cali_enabled) {
        int mv = 0;
        ESP_ERROR_CHECK(adc_cali_raw_to_voltage(ch->cali, raw, &mv));
        lua_pushinteger(L, mv);
        return 1;
    } else {
        lua_pushnil(L);
        lua_pushstring(L, "no_cali");
        return 2;
    }
}

static int l_adc_close(lua_State *L) {
    lua_adc_ch_t *ch = (lua_adc_ch_t*)luaL_checkudata(L, 1, "hw.adc.ch");
    l_adc_channel_gc(L);
    (void)ch;
    lua_pushboolean(L, 1);
    return 1;
}

// Lua binding
int LUA_OPEN_ADC(lua_State *L) {
    lua_newtable(L);
    lua_pushcfunction(L, l_adc_channel); lua_setfield(L, -2, "channel");
    lua_pushinteger(L, 1); lua_setfield(L, -2, "UNIT_1");
    lua_pushinteger(L, 2); lua_setfield(L, -2, "UNIT_2");
    return 1;
}
