#include "sys_os.h"

static void _UartInit(void) {
    // Inisialisasi UART
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 122,
        .source_clk = UART_SCLK_DEFAULT
    };

    uart_driver_install(UART_NUM_0, LUA_BUFFER_SIZE, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_0, &uart_config);
    uart_set_pin(UART_NUM_0, 1, 3, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}


// Fungsi reboot
static int L_OS_REBOOT(lua_State *L) {
    esp_restart();   // langsung restart ESP32
    return 0;        // gak ada return value
}

// Registrasi semua fungsi ke Lua
static const luaL_Reg OS_LIB[] = {
    {"reboot", L_OS_REBOOT},
    {NULL, NULL}
};

// Loader library OS
int LUA_OPEN_OS ( lua_State *L ) { _UartInit(); luaL_newlib(L, OS_LIB); return 1; }