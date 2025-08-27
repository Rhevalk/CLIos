#include "init.h"

int _API_BINDING_C(lua_State *L) 
{
   /*--------- SYSTEM -------------*/
   luaL_requiref(L, "os", LUA_OPEN_OS, 1);
   luaL_requiref(L, "fs", LUA_OPEN_FS, 1);

      /*--------- HARDWARE -------------*/
   luaL_requiref(L, "gpio", LUA_OPEN_GPIO, 1); 
   luaL_requiref(L, "pwm", LUA_OPEN_PWM, 1); 
   luaL_requiref(L, "adc", LUA_OPEN_ADC, 1); 
   luaL_requiref(L, "dac", LUA_OPEN_DAC, 1); 
 
    /*--------- FREERTOS -------------*/
   luaL_requiref(L, "task", LUA_OPEN_TASK, 1);
   luaL_requiref(L, "queue", LUA_OPEN_QUEUE, 1);
   luaL_requiref(L, "mutex", LUA_OPEN_MUTEX, 1);
   luaL_requiref(L, "semaphore", LUA_OPEN_SEMAPHORE, 1);
 
    /*--------- NETWORK -------------*/
   luaL_requiref(L, "wifi", LUA_OPEN_WIFI, 1);
   luaL_requiref(L, "net", LUA_OPEN_NET, 1);
   luaL_requiref(L, "http", LUA_OPEN_HTTP, 1);
   luaL_requiref(L, "httpd", LUA_OPEN_HTTPD, 1);
   luaL_requiref(L, "mdns", LUA_OPEN_MDNS, 1);
   luaL_requiref(L, "sc", LUA_OPEN_SOCKET, 1);
   luaL_requiref(L, "tls", LUA_OPEN_TLS, 1);
   luaL_requiref(L, "wsc", LUA_OPEN_WEBSOCKET, 1);

 
   /*--------- COMMUNICATION -------------*/
   luaL_requiref(L, "uart", LUA_OPEN_UART, 1);
   luaL_requiref(L, "spi", LUA_OPEN_SPI, 1);
   luaL_requiref(L, "i2c", LUA_OPEN_I2C, 1);
   luaL_requiref(L, "can", LUA_OPEN_CAN, 1);
   luaL_requiref(L, "rmt", LUA_OPEN_RMT, 1);

   /*--------- BLUETOOTH -------------*/
   luaL_requiref(L, "ble", LUA_OPEN_BLE, 1);


  lua_pop(L, 1);

  return 1;
}