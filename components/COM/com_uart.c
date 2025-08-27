// ============================= IMPLEMENTATION =============================
// u = uart.open(1, 115200, 17, 16)
// 
// while true do
//     local data = u:read(64, 1000) -- baca sampai 64 byte dengan timeout 1 detik
//     if #data > 0 then
//         print("RX:", data)
//         u:write("echo: " .. data)
//     end
// end
// ============================= IMPLEMENTATION =============================

#include "com_uart.h"

// ---- fungsi-fungsi method UART ----
static int F_WRITE(lua_State *L) {
    lua_uart_t *u=(lua_uart_t*)luaL_checkudata(L,1,"uart.handle");
    size_t n; 
    const char* buf = luaL_checklstring(L,2,&n);
    int wrote = uart_write_bytes(u->port, buf, n);
    lua_pushinteger(L, wrote);
    return 1;
}

static int F_READ(lua_State *L) {
    lua_uart_t *u=(lua_uart_t*)luaL_checkudata(L,1,"uart.handle");
    int len = luaL_checkinteger(L,2);
    int to_ms = luaL_optinteger(L,3,100);
    char *tmp = (char*)malloc(len);
    if(!tmp) return luaL_error(L,"oom");
    int got = uart_read_bytes(u->port, (uint8_t*)tmp, len, pdMS_TO_TICKS(to_ms));
    lua_pushlstring(L,tmp, got>0 ? got : 0);
    free(tmp);
    return 1;
}

static int F_FLUSH(lua_State *L) {
    lua_uart_t *u=(lua_uart_t*)luaL_checkudata(L,1,"uart.handle");
    uart_flush(u->port);
    lua_pushboolean(L,1);
    return 1;
}

static int F_SETBAUD(lua_State *L) {
    lua_uart_t *u=(lua_uart_t*)luaL_checkudata(L,1,"uart.handle");
    int b = luaL_checkinteger(L,2);
    ESP_ERROR_CHECK(uart_set_baudrate(u->port,b));
    lua_pushboolean(L,1);
    return 1;
}

static int F_CLOSE(lua_State *L) {
    lua_uart_t *u=(lua_uart_t*)luaL_checkudata(L,1,"uart.handle");
    if(u->installed){
        uart_driver_delete(u->port);
        u->installed=false;
    }
    lua_pushboolean(L,1);
    return 1;
}

// ---- fungsi open() ----
static int L_UART_OPEN(lua_State *L) {
    int port = luaL_checkinteger(L, 1);
    int baud = luaL_optinteger(L, 2, 115200);
    int tx   = luaL_optinteger(L, 3, -1);
    int rx   = luaL_optinteger(L, 4, -1);
    int rts  = luaL_optinteger(L, 5, -1);
    int cts  = luaL_optinteger(L, 6, -1);
    int buf  = luaL_optinteger(L, 7, 2048);

    lua_uart_t *u = (lua_uart_t*)lua_newuserdata(L, sizeof(*u));
    memset(u,0,sizeof(*u));
    u->port = port;

    uart_config_t cfg = {
        .baud_rate = baud,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    ESP_ERROR_CHECK(uart_driver_install(port, buf, buf, 0, NULL, 0));
    u->installed = true;
    ESP_ERROR_CHECK(uart_param_config(port, &cfg));
    ESP_ERROR_CHECK(uart_set_pin(port, tx, rx, rts, cts));

    if(luaL_newmetatable(L, "uart.handle")){
        luaL_Reg mt[] = {
            {"write",    F_WRITE},
            {"read",     F_READ},
            {"flush",    F_FLUSH},
            {"set_baud", F_SETBAUD},
            {"close",    F_CLOSE},
            {NULL,NULL}
        };
        luaL_setfuncs(L, mt, 0);
        lua_pushvalue(L,-1);
        lua_setfield(L,-2,"__index");
    }
    lua_setmetatable(L,-2);
    return 1;
}

// ---- library registration ----
static const luaL_Reg UART_LIB[] = {
    {"open", L_UART_OPEN},
    {NULL,NULL}
};

int LUA_OPEN_UART(lua_State *L) {
    luaL_newlib(L, UART_LIB);
    return 1;
}
