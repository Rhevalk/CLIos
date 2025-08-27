#include "com_can.h"

#if HAVE_TWAI

// ==== methods untuk can.handle ====
static int F_CAN_TX(lua_State *L) {
    lua_can_t *h=(lua_can_t*)luaL_checkudata(L,1,"can.handle");
    (void)h; // unused for now

    twai_message_t m={0};
    uint32_t id=(uint32_t)luaL_checkinteger(L,2);
    size_t n;
    const char* data=luaL_optlstring(L,3,"",&n);

    m.identifier=id;
    m.data_length_code=(n>8)?8:n;
    memcpy(m.data,data,m.data_length_code);

    ESP_ERROR_CHECK(twai_transmit(&m,pdMS_TO_TICKS(1000)));
    lua_pushboolean(L,1);
    return 1;
}

static int F_CAN_RX(lua_State *L) {
    lua_can_t *h=(lua_can_t*)luaL_checkudata(L,1,"can.handle");
    (void)h;

    twai_message_t m;
    if(twai_receive(&m,pdMS_TO_TICKS(1000))==ESP_OK){
        lua_pushinteger(L,m.identifier);
        lua_pushlstring(L,(const char*)m.data,m.data_length_code);
        return 2;
    }
    lua_pushnil(L);
    return 1;
}

static int F_CAN_STOP(lua_State *L) {
    lua_can_t *h=(lua_can_t*)luaL_checkudata(L,1,"can.handle");
    (void)h;

    twai_stop();
    twai_driver_uninstall();

    lua_pushboolean(L,1);
    return 1;
}

// ==== konstruktor/start ====
static int L_CAN_START(lua_State *L) {
    int tx = luaL_checkinteger(L,1);
    int rx = luaL_checkinteger(L,2);
    int bitrate = luaL_optinteger(L,3,500); // kbit/s

    twai_general_config_t g = TWAI_GENERAL_CONFIG_DEFAULT(
        (gpio_num_t)tx, (gpio_num_t)rx, TWAI_MODE_NORMAL);

    twai_timing_config_t t;   // declare here

    if (bitrate == 1000) {
        t = (twai_timing_config_t)TWAI_TIMING_CONFIG_1MBITS();
    } else if (bitrate == 800) {
        t = (twai_timing_config_t)TWAI_TIMING_CONFIG_800KBITS();
    } else if (bitrate == 250) {
        t = (twai_timing_config_t)TWAI_TIMING_CONFIG_250KBITS();
    } else {
        t = (twai_timing_config_t)TWAI_TIMING_CONFIG_500KBITS();
    }

    twai_filter_config_t f = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    ESP_ERROR_CHECK(twai_driver_install(&g, &t, &f));
    ESP_ERROR_CHECK(twai_start());

    lua_can_t *h = (lua_can_t*)lua_newuserdata(L, sizeof(*h));
    memset(h, 0, sizeof(*h));
    h->started = true;

    if(luaL_newmetatable(L,"can.handle")){
        luaL_Reg mt[]={
            {"tx",   F_CAN_TX},
            {"rx",   F_CAN_RX},
            {"stop", F_CAN_STOP},
            {NULL,NULL}
        };
        luaL_setfuncs(L, mt, 0);
        lua_pushvalue(L, -1);
        lua_setfield(L, -2, "__index");
    }
    lua_setmetatable(L, -2);
    return 1;
}


#else // !HAVE_TWAI

static int L_CAN_START(lua_State *L) {
    return luaL_error(L,"TWAI not available in this IDF");
}

#endif // HAVE_TWAI

// ==== library registration ====
static const luaL_Reg CAN_LIB[] = {
    {"start", L_CAN_START},
    {NULL,NULL}
};

int LUA_OPEN_CAN(lua_State *L) {
    luaL_newlib(L, CAN_LIB);
    return 1;
}
