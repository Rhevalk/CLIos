#include "os.h"
// Fungsi reboot
static int L_OS_REBOOT(lua_State *L) {
    esp_restart();   // langsung restart ESP32
    return 0;        // gak ada return value
}

// Fungsi sleep via GPIO
// static int L_OS_SGPIO(lua_State *L) {
//   gpio_num_t pin = (gpio_num_t)luaL_checkinteger(L, 1);
//   esp_sleep_enable_ext0_wakeup(pin, 0);
//   printf("Woke up from GPIO %d\n", pin);
//   esp_deep_sleep_start();
//   return 0;
// }

// Fungsi sleep via UART
// static int L_OS_SUART(lua_State *L) {
//   uint8_t uart_num = luaL_checkinteger(L, 1);
//   esp_sleep_enable_uart_wakeup(uart_num);
//   printf("Woke up from UART %d\n", uart_num);
//   esp_light_sleep_start();
//   return 0;
// }

// Registrasi semua fungsi ke Lua
static const luaL_Reg OS_LIB[] = {
    {"reboot", L_OS_REBOOT},
    //{"sgpio",  L_OS_SGPIO},
    //{"suart",  L_OS_SUART},
    {NULL, NULL}
};

// Loader library OS
int LUA_OPEN_OS ( lua_State *L ) { luaL_newlib(L, OS_LIB); return 1; }