// ============================= IMPLEMENTATION =============================
// bus = i2c.new_bus(0, 21, 22, 400)
// 
// dev = bus:add_device(0x3C)
// 
// dev:tx("\x00\xAF")   -- kirim command 0xAF
// 
// local data = dev:rx(4)
// print("RX:", string.byte(data,1,4))
// 
// local resp = dev:wrrd("\x00", 2)  -- tulis register 0x00 lalu baca 2 byte
// print("WRRD:", string.byte(resp,1,2))
// 
// dev:remove()
// 
// bus:close()
// ============================= IMPLEMENTATION =============================


#include "com_i2s.h"

// ==== method untuk i2s.handle ====
static int F_I2S_WRITE(lua_State *L) {
    lua_i2s_t *h=(lua_i2s_t*)luaL_checkudata(L,1,"i2s.handle");
    size_t n; 
    const char* buf=luaL_checklstring(L,2,&n);
    size_t wrote=0;

#if HAVE_I2S_V5
    size_t out=0;
    ESP_ERROR_CHECK(i2s_channel_write(h->tx, buf, n, &out, portMAX_DELAY));
    wrote=out;
#else
    size_t out=0;
    i2s_write(h->port, buf, n, &out, portMAX_DELAY);
    wrote=out;
#endif

    lua_pushinteger(L,(lua_Integer)wrote);
    return 1;
}

static int F_I2S_READ(lua_State *L) {
    lua_i2s_t *h=(lua_i2s_t*)luaL_checkudata(L,1,"i2s.handle");
    int n=luaL_checkinteger(L,2);
    char *buf=(char*)malloc(n);
    size_t got=0;
    if(!buf) return luaL_error(L,"oom");

#if HAVE_I2S_V5
    ESP_ERROR_CHECK(i2s_channel_read(h->rx, buf, n, &got, portMAX_DELAY));
#else
    i2s_read(h->port, buf, n, &got, portMAX_DELAY);
#endif

    lua_pushlstring(L, buf, got);
    free(buf);
    return 1;
}

static int F_I2S_CLOSE(lua_State *L) {
    lua_i2s_t *h=(lua_i2s_t*)luaL_checkudata(L,1,"i2s.handle");
#if HAVE_I2S_V5
    if(h->tx) { 
        i2s_channel_disable(h->tx); 
        i2s_del_channel(h->tx); 
        h->tx=NULL; 
    }
    if(h->rx) { 
        i2s_channel_disable(h->rx); 
        i2s_del_channel(h->rx); 
        h->rx=NULL; 
    }
#else
    if(h->installed){ 
        i2s_driver_uninstall(h->port); 
        h->installed=false; 
    }
#endif
    lua_pushboolean(L,1);
    return 1;
}

// ==== konstruktor ====
static int L_I2S_NEW(lua_State *L) {
    int sample_rate = luaL_optinteger(L,1,16000);
    int bits        = luaL_optinteger(L,2,16);
    int mclk        = luaL_optinteger(L,3,0); // 0=auto
    int bclk        = luaL_checkinteger(L,4);
    int ws          = luaL_checkinteger(L,5);
    int dout        = luaL_optinteger(L,6,-1);
    int din         = luaL_optinteger(L,7,-1);

    lua_i2s_t *h=(lua_i2s_t*)lua_newuserdata(L,sizeof(*h));
    memset(h,0,sizeof(*h));

#if HAVE_I2S_V5
    i2s_chan_config_t c = I2S_CHANNEL_DEFAULT_CONFIG((i2s_port_t)0, I2S_ROLE_MASTER);
    c.auto_clear = true;
    ESP_ERROR_CHECK(i2s_new_channel(&c, &h->tx, &h->rx));

    i2s_data_bit_width_t width =
        (bits == 16) ? I2S_DATA_BIT_WIDTH_16BIT : I2S_DATA_BIT_WIDTH_32BIT;

    i2s_std_config_t sc = {
        .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(sample_rate),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(width, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED, // kalau mau aktifkan mclk, isi sesuai pin
            .bclk = bclk,
            .ws   = ws,
            .dout = dout,
            .din  = (din >= 0) ? din : I2S_GPIO_UNUSED,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv   = false,
            },
        },
    };

    // API baru (v5)
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(h->tx, &sc));
    if (din >= 0) ESP_ERROR_CHECK(i2s_channel_init_std_mode(h->rx, &sc));

    ESP_ERROR_CHECK(i2s_channel_enable(h->tx));
    if (din >= 0) ESP_ERROR_CHECK(i2s_channel_enable(h->rx));

    (void)mclk; // suppress unused warning

#else
    i2s_config_t cfg = {
        .mode = I2S_MODE_MASTER | I2S_MODE_TX | ((din>=0)?I2S_MODE_RX:0),
        .sample_rate     = sample_rate,
        .bits_per_sample = (i2s_bits_per_sample_t)bits,
        .channel_format  = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_I2S,
        .intr_alloc_flags= 0,
        .dma_buf_count   = 6,
        .dma_buf_len     = 256,
        .use_apll        = false
    };
    i2s_pin_config_t p={
        .mck_io_num  = mclk,
        .bck_io_num  = bclk,
        .ws_io_num   = ws,
        .data_out_num= dout,
        .data_in_num = din
    };
    int port=0;
    ESP_ERROR_CHECK(i2s_driver_install(port,&cfg,0,NULL));
    ESP_ERROR_CHECK(i2s_set_pin(port,&p));
    h->port=port;
    h->installed=true;
#endif

    if(luaL_newmetatable(L,"i2s.handle")){
        luaL_Reg mt[] = {
            {"write", F_I2S_WRITE},
            {"read",  F_I2S_READ},
            {"close", F_I2S_CLOSE},
            {NULL,NULL}
        };
        luaL_setfuncs(L,mt,0);
        lua_pushvalue(L,-1);
        lua_setfield(L,-2,"__index");
    }
    lua_setmetatable(L,-2);
    return 1;
}

// ==== library registration ====
static const luaL_Reg I2S_LIB[] = {
    {"new", L_I2S_NEW},
    {NULL,NULL}
};

int LUA_OPEN_I2S(lua_State *L) {
    luaL_newlib(L, I2S_LIB);
    return 1;
}
