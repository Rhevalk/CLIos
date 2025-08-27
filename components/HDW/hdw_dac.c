// ============================= IMPLEMENTATION =============================
// d = dac.new(dac.CH0)
// 
// -- sweep tegangan naik
// for i = 0, 255, 16 do
//     d:write(i)
//     print("DAC set ke:", i)
//     tmr.delay(100000) -- 100ms
// end
// 
// -- close
// d:free()
// ============================= IMPLEMENTATION =============================

#include "hdw_dac.h"

#if HAVE_DAC

// --- method: write ---
static int l_dac_write(lua_State *L) {
    lua_dac_t *d = (lua_dac_t*)luaL_checkudata(L, 1, "hw.dac");
    uint8_t v = (uint8_t)luaL_checkinteger(L, 2); // 0..255 (8bit DAC)
    ESP_ERROR_CHECK(dac_oneshot_output_voltage(d->h, v));
    lua_pushboolean(L, 1);
    return 1;
}

// --- method: free/close ---
static int l_dac_free(lua_State *L) {
    lua_dac_t *d = (lua_dac_t*)luaL_checkudata(L, 1, "hw.dac");
    if (d->h) {
        dac_oneshot_del_channel(d->h);
        d->h = NULL;
    }
    lua_pushboolean(L, 1);
    return 1;
}
#endif // HAVE_DAC

// --- constructor ---
static int l_dac_new(lua_State *L) {
#if !HAVE_DAC
    return luaL_error(L, "DAC driver not available in this IDF");
#else
    int channel = luaL_checkinteger(L, 1); // DAC_CHAN_0 / DAC_CHAN_1
    lua_dac_t *d = (lua_dac_t*)lua_newuserdata(L, sizeof(*d));
    memset(d, 0, sizeof(*d));

    d->ch = (dac_channel_t)channel;
    dac_oneshot_config_t cfg = { .chan_id = d->ch };
    ESP_ERROR_CHECK(dac_oneshot_new_channel(&cfg, &d->h));

    // buat metatable
    if (luaL_newmetatable(L, "hw.dac")) {
        lua_pushcfunction(L, l_dac_write);
        lua_setfield(L, -2, "write");

        lua_pushcfunction(L, l_dac_free);
        lua_setfield(L, -2, "free");

        lua_pushvalue(L, -1);
        lua_setfield(L, -2, "__index");
    }
    lua_setmetatable(L, -2);
    return 1;
#endif
}

// --- library open ---
int LUA_OPEN_DAC(lua_State *L) {
    lua_newtable(L);
    lua_pushcfunction(L, l_dac_new);
    lua_setfield(L, -2, "new");
#if HAVE_DAC
    lua_pushinteger(L, DAC_CHAN_0); lua_setfield(L, -2, "CH0");
    lua_pushinteger(L, DAC_CHAN_1); lua_setfield(L, -2, "CH1");
#endif
    return 1;
}
