#include "com_rmt.h"
// ---------- forward decl ----------
static int F_SEND_RAW(lua_State *L);
static int F_CLOSE(lua_State *L);
static int L_RMT_TX_NEW(lua_State *L);

// ---------- send_raw (method) ----------
static int F_SEND_RAW(lua_State *L) {
    lua_rmt_t *h = (lua_rmt_t*)luaL_checkudata(L, 1, "rmt.tx");

#if HAVE_RMT_V5
    // Arg 2 boleh string (raw bytes) atau table durasi {hi,lo, hi,lo, ...}
    int t = lua_type(L, 2);
    if (t == LUA_TSTRING) {
        size_t len = 0;
        const char *buf = lua_tolstring(L, 2, &len);
        rmt_transmit_config_t tc = {0};
        ESP_ERROR_CHECK(rmt_transmit(h->ch, h->enc, buf, len, &tc));
        ESP_ERROR_CHECK(rmt_tx_wait_all_done(h->ch, portMAX_DELAY));
    } else if (t == LUA_TTABLE) {
        int n = (int)lua_rawlen(L, 2);
        if (n <= 0 || (n & 1)) {
            return luaL_error(L, "table must contain even count: {hi,lo,...}");
        }
        int pairs = n / 2;
        rmt_symbol_word_t *syms = (rmt_symbol_word_t*)calloc(pairs, sizeof(rmt_symbol_word_t));
        if (!syms) return luaL_error(L, "oom");

        for (int i = 1, si = 0; i <= n; i += 2, si++) {
            lua_rawgeti(L, 2, i);
            lua_rawgeti(L, 2, i + 1);
            uint32_t hi = (uint32_t)luaL_checkinteger(L, -2);
            uint32_t lo = (uint32_t)luaL_checkinteger(L, -1);
            lua_pop(L, 2);

            syms[si].level0 = 1;
            syms[si].duration0 = hi;
            syms[si].level1 = 0;
            syms[si].duration1 = lo;
        }
        rmt_transmit_config_t tc = {0};
        ESP_ERROR_CHECK(rmt_transmit(h->ch, h->enc, syms, pairs * sizeof(*syms), &tc));
        ESP_ERROR_CHECK(rmt_tx_wait_all_done(h->ch, portMAX_DELAY));
        free(syms);
    } else {
        return luaL_error(L, "arg 2 must be string or table");
    }
#else
    // Legacy driver: hanya table item (level/duration)
    luaL_checktype(L, 2, LUA_TTABLE);
    int n = (int)lua_rawlen(L, 2);
    if (n <= 0 || (n & 1)) {
        return luaL_error(L, "table must contain even count: {hi,lo,...}");
    }
    int pairs = n / 2;
    rmt_item32_t *items = (rmt_item32_t*)calloc(pairs, sizeof(rmt_item32_t));
    if (!items) return luaL_error(L, "oom");

    for (int i = 1, si = 0; i <= n; i += 2, si++) {
        lua_rawgeti(L, 2, i);
        lua_rawgeti(L, 2, i + 1);
        uint16_t hi = (uint16_t)luaL_checkinteger(L, -2);
        uint16_t lo = (uint16_t)luaL_checkinteger(L, -1);
        lua_pop(L, 2);

        items[si].level0 = 1;
        items[si].duration0 = hi;
        items[si].level1 = 0;
        items[si].duration1 = lo;
    }
    ESP_ERROR_CHECK(rmt_write_items(h->ch, items, pairs, true));
    free(items);
#endif

    lua_pushboolean(L, 1);
    return 1;
}

// ---------- close (method) ----------
static int F_CLOSE(lua_State *L) {
    lua_rmt_t *h = (lua_rmt_t*)luaL_checkudata(L, 1, "rmt.tx");
#if HAVE_RMT_V5
    if (h->enc) { rmt_del_encoder(h->enc); h->enc = NULL; }
    if (h->ch)  { rmt_disable(h->ch); rmt_del_channel(h->ch); h->ch = NULL; }
#else
    if (h->installed) { rmt_driver_uninstall(h->ch); h->installed = false; }
#endif
    lua_pushboolean(L, 1);
    return 1;
}

// ---------- constructor ----------
static int L_RMT_TX_NEW(lua_State *L) {
    int pin = luaL_checkinteger(L, 1);
    int resolution_hz = luaL_optinteger(L, 2, 10000000); // default 10 MHz tick

    lua_rmt_t *h = (lua_rmt_t*)lua_newuserdata(L, sizeof(*h));
    memset(h, 0, sizeof(*h));

#if HAVE_RMT_V5
    rmt_tx_channel_config_t c = {
        .gpio_num = pin,
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .mem_block_symbols = 64,
        .resolution_hz = (uint32_t)resolution_hz,
        .trans_queue_depth = 4
    };
    ESP_ERROR_CHECK(rmt_new_tx_channel(&c, &h->ch));

    // config encoder (pakai {} / compound literal agar tak kena "excess elements")
    rmt_copy_encoder_config_t ec = {};
    ESP_ERROR_CHECK(rmt_new_copy_encoder(&ec, &h->enc));

    ESP_ERROR_CHECK(rmt_enable(h->ch));
#else
    rmt_config_t c = {
        .rmt_mode = RMT_MODE_TX,
        .channel = RMT_CHANNEL_0,  // bisa dibuat argumen kalau mau
        .gpio_num = pin,
        .clk_div = 80,
        .mem_block_num = 1,
    };
    ESP_ERROR_CHECK(rmt_config(&c));
    ESP_ERROR_CHECK(rmt_driver_install(c.channel, 0, 0));
    h->ch = c.channel;
    h->installed = true;
#endif

    if (luaL_newmetatable(L, "rmt.tx")) {
        luaL_Reg mt[] = {
            {"send_raw", F_SEND_RAW},
            {"close",    F_CLOSE},
            {NULL, NULL}
        };
        luaL_setfuncs(L, mt, 0);
        lua_pushvalue(L, -1);
        lua_setfield(L, -2, "__index");
    }
    lua_setmetatable(L, -2);
    return 1;
}

// ---------- library open ----------
static const luaL_Reg RMT_LIB[] = {
    {"tx_new", L_RMT_TX_NEW},
    {NULL, NULL}
};

int LUA_OPEN_RMT(lua_State *L) {
    luaL_newlib(L, RMT_LIB);
    return 1;
}
