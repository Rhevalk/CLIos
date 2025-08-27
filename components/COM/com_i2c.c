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

#include "com_i2c.h"

// ==== forward declaration untuk i2c.dev methods ====
static int F_I2C_DEV_TX(lua_State *L);
static int F_I2C_DEV_RX(lua_State *L);
static int F_I2C_DEV_WRRD(lua_State *L);
static int F_I2C_DEV_REMOVE(lua_State *L);

// ==== method untuk i2c.bus ====
static int F_I2C_BUS_CLOSE(lua_State *L) {
    lua_i2c_bus_t *b = (lua_i2c_bus_t*)luaL_checkudata(L,1,"i2c.bus");
#if HAVE_I2C_V5
    i2c_del_master_bus(b->bus);
#else
    i2c_driver_delete(b->port);
#endif
    lua_pushboolean(L,1);
    return 1;
}

static int F_I2C_BUS_ADD_DEV(lua_State *L) {
    lua_i2c_bus_t *b = (lua_i2c_bus_t*)luaL_checkudata(L,1,"i2c.bus");
    int addr = luaL_checkinteger(L,2);
    int khz  = luaL_optinteger(L,3,400);

    lua_i2c_dev_t *d=(lua_i2c_dev_t*)lua_newuserdata(L,sizeof(*d));
    memset(d,0,sizeof(*d));

#if HAVE_I2C_V5
    i2c_device_config_t dc = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = addr,
        .scl_speed_hz    = khz * 1000
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(b->bus,&dc,&d->dev));
#else
    d->port=b->port;
    d->addr=addr;
    (void)khz;
#endif

    if(luaL_newmetatable(L,"i2c.dev")){
        static const luaL_Reg dmt[] = {
            {"tx",     F_I2C_DEV_TX},
            {"rx",     F_I2C_DEV_RX},
            {"wrrd",   F_I2C_DEV_WRRD},
            {"remove", F_I2C_DEV_REMOVE},
            {NULL,NULL}
        };
        luaL_setfuncs(L,dmt,0);
        lua_pushvalue(L,-1);
        lua_setfield(L,-2,"__index");
    }
    lua_setmetatable(L,-2);
    return 1;
}

// ==== method untuk i2c.dev ====
static int F_I2C_DEV_TX(lua_State *L) {
    lua_i2c_dev_t *d=(lua_i2c_dev_t*)luaL_checkudata(L,1,"i2c.dev");
    size_t n; const char* buf=luaL_checklstring(L,2,&n);
    int to=luaL_optinteger(L,3,1000);
#if HAVE_I2C_V5
    ESP_ERROR_CHECK(i2c_master_transmit(d->dev,(const uint8_t*)buf,n,to));
#else
    i2c_cmd_handle_t cmd=i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd,(d->addr<<1)|I2C_MASTER_WRITE,true);
    i2c_master_write(cmd,(const uint8_t*)buf,n,true);
    i2c_master_stop(cmd);
    ESP_ERROR_CHECK(i2c_master_cmd_begin(d->port,cmd,pdMS_TO_TICKS(to)));
    i2c_cmd_link_delete(cmd);
#endif
    lua_pushboolean(L,1);
    return 1;
}

static int F_I2C_DEV_RX(lua_State *L) {
    lua_i2c_dev_t *d=(lua_i2c_dev_t*)luaL_checkudata(L,1,"i2c.dev");
    int n=luaL_checkinteger(L,2);
    int to=luaL_optinteger(L,3,1000);
    uint8_t *buf=(uint8_t*)malloc(n);
    if(!buf) return luaL_error(L,"oom");
#if HAVE_I2C_V5
    ESP_ERROR_CHECK(i2c_master_receive(d->dev,buf,n,to));
#else
    i2c_cmd_handle_t cmd=i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd,(d->addr<<1)|I2C_MASTER_READ,true);
    if(n>1) i2c_master_read(cmd,buf,n-1,I2C_MASTER_ACK);
    i2c_master_read_byte(cmd,buf+n-1,I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    ESP_ERROR_CHECK(i2c_master_cmd_begin(d->port,cmd,pdMS_TO_TICKS(to)));
    i2c_cmd_link_delete(cmd);
#endif
    lua_pushlstring(L,(const char*)buf,n);
    free(buf);
    return 1;
}

static int F_I2C_DEV_WRRD(lua_State *L) {
    lua_i2c_dev_t *d=(lua_i2c_dev_t*)luaL_checkudata(L,1,"i2c.dev");
    size_t wn; const char*w=luaL_checklstring(L,2,&wn);
    int rn=luaL_checkinteger(L,3);
    int to=luaL_optinteger(L,4,1000);
    uint8_t *r=(uint8_t*)malloc(rn);
    if(!r) return luaL_error(L,"oom");
#if HAVE_I2C_V5
    ESP_ERROR_CHECK(i2c_master_transmit_receive(d->dev,(const uint8_t*)w,wn,r,rn,to));
#else
    i2c_cmd_handle_t cmd=i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd,(d->addr<<1)|I2C_MASTER_WRITE,true);
    i2c_master_write(cmd,(const uint8_t*)w,wn,true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd,(d->addr<<1)|I2C_MASTER_READ,true);
    if(rn>1) i2c_master_read(cmd,r,rn-1,I2C_MASTER_ACK);
    i2c_master_read_byte(cmd,r+rn-1,I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    ESP_ERROR_CHECK(i2c_master_cmd_begin(d->port,cmd,pdMS_TO_TICKS(to)));
    i2c_cmd_link_delete(cmd);
#endif
    lua_pushlstring(L,(const char*)r,rn);
    free(r);
    return 1;
}

static int F_I2C_DEV_REMOVE(lua_State *L) {
#if HAVE_I2C_V5
    lua_i2c_dev_t *d=(lua_i2c_dev_t*)luaL_checkudata(L,1,"i2c.dev");
    i2c_master_bus_rm_device(d->dev);
#endif
    lua_pushboolean(L,1);
    return 1;
}

// ==== konstruktor bus ====
static int L_I2C_BUS_NEW(lua_State *L) {
    int port = luaL_optinteger(L,1,0);
    int sda = luaL_checkinteger(L,2);
    int scl = luaL_checkinteger(L,3);
    int khz = luaL_optinteger(L,4,400);

#if HAVE_I2C_V5
    (void)khz; // suppress warning di v5

    i2c_master_bus_config_t cfg = {
        .clk_source=I2C_CLK_SRC_DEFAULT,
        .i2c_port = port,
        .sda_io_num = sda,
        .scl_io_num = scl,
        .glitch_ignore_cnt=7,
        .flags.enable_internal_pullup = 1
    };
    i2c_master_bus_handle_t bus;
    ESP_ERROR_CHECK(i2c_new_master_bus(&cfg,&bus));
    lua_i2c_bus_t *b=(lua_i2c_bus_t*)lua_newuserdata(L,sizeof(*b));
    b->bus=bus;
#else
    i2c_config_t cfg = {
        .mode=I2C_MODE_MASTER,
        .sda_io_num=sda,
        .scl_io_num=scl,
        .sda_pullup_en=true,
        .scl_pullup_en=true,
        .master.clk_speed = khz*1000
    };
    ESP_ERROR_CHECK(i2c_param_config(port,&cfg));
    ESP_ERROR_CHECK(i2c_driver_install(port,I2C_MODE_MASTER,0,0,0));
    lua_i2c_bus_t *b=(lua_i2c_bus_t*)lua_newuserdata(L,sizeof(*b));
    b->port=port;
#endif

    if(luaL_newmetatable(L,"i2c.bus")){
        static const luaL_Reg bmt[] = {
            {"add_device", F_I2C_BUS_ADD_DEV},
            {"close",      F_I2C_BUS_CLOSE},
            {NULL,NULL}
        };
        luaL_setfuncs(L,bmt,0);
        lua_pushvalue(L,-1);
        lua_setfield(L,-2,"__index");
    }
    lua_setmetatable(L,-2);
    return 1;
}

// ==== library registration ====
static const luaL_Reg I2C_LIB[] = {
    {"new_bus", L_I2C_BUS_NEW},
    {NULL,NULL}
};

int LUA_OPEN_I2C(lua_State *L) {
    luaL_newlib(L, I2C_LIB);
    return 1;
}
