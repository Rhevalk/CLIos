// ============================= IMPLEMENTATION =============================
// spi.bus_init(esp.SPI2_HOST, 23, 19, 18)
// 
// local dev = spi.add_device(esp.SPI2_HOST, 5, 10, 0)
// 
// local cmd = string.char(0x9F)
// local rx = dev:xfer(cmd, 3)
// print("Flash ID:", string.byte(rx,1,3))
// 
// dev:remove()
// spi.bus_free(esp.SPI2_HOST)
// ============================= IMPLEMENTATION =============================

#include "com_spi.h"

// ==== method untuk spi.dev ====
static int F_SPI_DEV_TXRX(lua_State *L) {
    lua_spi_dev_t *d=(lua_spi_dev_t*)luaL_checkudata(L,1,"spi.dev");
    size_t nt; 
    const char* tx=luaL_optlstring(L,2,NULL,&nt);
    int rxlen = luaL_optinteger(L,3,(int)nt);

    uint8_t *rx=NULL;
    spi_transaction_t t={0};

    if(tx){
        t.length = nt*8;
        t.tx_buffer = tx;
    }
    if(rxlen > 0){
        rx = (uint8_t*)malloc(rxlen);
        if(!rx) return luaL_error(L,"oom");
        t.rxlength = rxlen*8;
        t.rx_buffer = rx;
    }

    ESP_ERROR_CHECK(spi_device_transmit(d->dev,&t));

    if(rx){
        lua_pushlstring(L,(const char*)rx,rxlen);
        free(rx);
    } else {
        lua_pushlstring(L,"",0);
    }
    return 1;
}

static int F_SPI_DEV_REMOVE(lua_State *L) {
    lua_spi_dev_t *d=(lua_spi_dev_t*)luaL_checkudata(L,1,"spi.dev");
    spi_bus_remove_device(d->dev);
    lua_pushboolean(L,1);
    return 1;
}

// ==== bus functions ====
static int L_SPI_BUS_INIT(lua_State *L) {
    int host = luaL_optinteger(L,1,SPI2_HOST); // SPI2_HOST=HSPI, SPI3_HOST=VSPI on some chips
    int mosi = luaL_optinteger(L,2,-1);
    int miso = luaL_optinteger(L,3,-1);
    int sclk = luaL_optinteger(L,4,-1);
    int dma  = luaL_optinteger(L,5,SPI_DMA_CH_AUTO);

    spi_bus_config_t bus={
        .mosi_io_num=mosi,
        .miso_io_num=miso,
        .sclk_io_num=sclk,
        .quadwp_io_num=-1,
        .quadhd_io_num=-1
    };
    ESP_ERROR_CHECK(spi_bus_initialize(host,&bus,dma));
    lua_pushboolean(L,1);
    return 1;
}

static int L_SPI_BUS_FREE(lua_State *L) {
    int host = luaL_checkinteger(L,1);
    ESP_ERROR_CHECK(spi_bus_free(host));
    lua_pushboolean(L,1);
    return 1;
}

static int L_SPI_ADD_DEVICE(lua_State *L) {
    int host = luaL_checkinteger(L,1);
    int cs   = luaL_checkinteger(L,2);
    int mhz  = luaL_optinteger(L,3,10);
    int mode = luaL_optinteger(L,4,0);

    lua_spi_dev_t *d=(lua_spi_dev_t*)lua_newuserdata(L,sizeof(*d));
    memset(d,0,sizeof(*d));

    spi_device_interface_config_t cfg = {
        .clock_speed_hz = mhz*1000*1000,
        .mode           = mode,
        .spics_io_num   = cs,
        .queue_size     = 1,
    };
    ESP_ERROR_CHECK(spi_bus_add_device(host,&cfg,&d->dev));

    if(luaL_newmetatable(L,"spi.dev")){
        luaL_Reg mt[] = {
            {"xfer",   F_SPI_DEV_TXRX},
            {"remove", F_SPI_DEV_REMOVE},
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
static const luaL_Reg SPI_LIB[] = {
    {"bus_init",  L_SPI_BUS_INIT},
    {"bus_free",  L_SPI_BUS_FREE},
    {"add_device",L_SPI_ADD_DEVICE},
    {NULL,NULL}
};

int LUA_OPEN_SPI(lua_State *L) {
    luaL_newlib(L,SPI_LIB);
    return 1;
}