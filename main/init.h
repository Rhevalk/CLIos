#pragma once

#include "lua.h" // Include necessary headers for Lua
#include "lualib.h" // Include necessary headers for Lua libraries
#include "lauxlib.h" // Include necessary headers for Lua auxiliary functions


/*-------- SYSTEM --------------*/
#include "sys_littlefs.h"
#include "sys_os.h"

/*-------- HARDWARE -------------*/
#include "hdw_gpio.h"
#include "hdw_pwm.h"
#include "hdw_adc.h"
#include "hdw_dac.h"

/*-------- FREERTOS -------------*/
#include "fos_task.h"
#include "fos_queue.h"
#include "fos_mutex.h"
#include "fos_semaphore.h"

/*-------- NEYWORKING ------------*/
#include "net_wifi.h"
#include "net_http.h"
#include "net_httpd.h"
#include "net_mdns.h"
#include "net_net.h"
#include "net_socket.h"
#include "net_tls.h"
#include "net_websocket.h"


/*-------- COMMUNICATION ------------*/
#include "com_uart.h"
#include "com_spi.h"
#include "com_i2c.h"
#include "com_can.h"
#include "com_rmt.h"

/*--------- BLUETOOTH -------------*/
#include "ble_ble.h"

int _API_BINDING_C(lua_State *L);