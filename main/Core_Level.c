#include <stdio.h>
#include <string.h>
#include <stdatomic.h>


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"

#include "lua.h" // Include necessary headers for Lua
#include "lualib.h" // Include necessary headers for Lua libraries
#include "lauxlib.h" // Include necessary headers for Lua auxiliary functions

#include "global.h"
#include "gpio.h"
#include "littlefs.h"
//#include "os.h"
#include "task.h"

lua_State *L = NULL;

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

    uart_driver_install(UART_NUM_0, 1024, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_0, &uart_config);
    uart_set_pin(UART_NUM_0, 1, 3, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}


// Hook function untuk menghentikan loop di Lua
atomic_bool stop_flag = false;
void interrupt_hook(lua_State *L, lua_Debug *ar) {
    if (atomic_load(&stop_flag)) {
        luaL_error(L, "Execution interrupted");
    }
}

TaskHandle_t consumerTaskHandle;

atomic_bool active_index = false;

char buffer_perline[2][CHAR_BUFFER_SIZE];
uint8_t buffer_perline_len[2];

char buffer_multiline[LUA_BUFFER_SIZE];     // buffer multiline
uint8_t buffer_multiline_len = 0;    // panjang isi

void _HandleLuaCommand(void *pvParameters) {
    for (;;) {
        // tunggu notifikasi dari producer
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        // ambil buffer aktif
        bool idx = atomic_load(&active_index);
        const char *buffer_local = buffer_perline[idx]; 

        // cek ukuran
        uint8_t len = buffer_perline_len[idx];

        // tambahkan ke multiline buffer
        memcpy(buffer_multiline + buffer_multiline_len, buffer_local, len);
        buffer_multiline_len += len;
        buffer_multiline[buffer_multiline_len++] = '\n';
        buffer_multiline[buffer_multiline_len]   = '\0';

        // reset state Lua VM + hook interrupt
        lua_settop(L, 0);
        lua_sethook(L, interrupt_hook, LUA_MASKCOUNT | LUA_MASKCALL | LUA_MASKRET, 100); //1000

        // coba compile & jalankan
        int status = luaL_loadbuffer(L, buffer_multiline, buffer_multiline_len, "line");
        if (status == LUA_OK) {
            status = lua_pcall(L, 0, LUA_MULTRET, 0);
            if (status != LUA_OK) {
                LOG_E("Runtime error: %s", lua_tostring(L, -1));
                lua_pop(L, 1);
                atomic_store(&stop_flag, false);
                continue;
            }
            buffer_multiline_len = 0;
        }
        else {
            const char *err = lua_tostring(L, -1);

            if (err && strstr(err, "near <eof>")) {
                if (strstr(err, "expected")) {
                    LOG_I("...");
                    lua_pop(L, 1);
                    continue; // incomplete → tunggu baris berikutnya
                }

                // coba lagi dengan newline tambahan
                buffer_multiline[buffer_multiline_len++] = '\n';
                buffer_multiline[buffer_multiline_len]   = '\0';
                lua_settop(L, 0);

                if (luaL_loadbuffer(L, buffer_multiline, buffer_multiline_len, "line") == LUA_OK) {
                    LOG_I("...");
                    lua_pop(L, 1);
                    continue;
                }
            }

            // ❌ syntax error
            LOG_E("Syntax error: %s", err);
            lua_pop(L, 1);
            buffer_multiline_len = 0;
        }

        // reset flag & hook
        atomic_store(&stop_flag, false);
        lua_sethook(L, NULL, 0, 0);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// Task untuk CLI
void _ShellTask(void *pvParameters) {
    char buffer_input[CHAR_BUFFER_SIZE];
    size_t index = 0;
    uint8_t data;

    for (;;) {
        if (uart_read_bytes(UART_NUM_0, &data, 1, pdMS_TO_TICKS(10)) > 0) {
            char c = (char)data;

            // tombol quit
            if (c == 'q') {
                if (!atomic_load(&stop_flag)) {
                    atomic_store(&stop_flag, true);
                    LOG_I("Interrupt signal sent to Lua");
                }
                continue;
            }

            // end of line
            if (c == '\r' || c == '\n') {
                if (index > 0) {
                    buffer_input[index] = '\0';

                    bool next_index = !atomic_load(&active_index);

                    size_t len = strlen(buffer_input);
                    if (len >= CHAR_BUFFER_SIZE) {
                        len = CHAR_BUFFER_SIZE - 1;
                    }
                    memcpy(buffer_perline[next_index], buffer_input, len);
                    buffer_perline[next_index][len] = '\0';
                    buffer_perline_len[next_index] = len;

                    atomic_store(&active_index, next_index);
                    xTaskNotifyGive(consumerTaskHandle);

                    LOG_I("%s", buffer_input);
                    index = 0;
                }
            }
            // karakter biasa
            else if (index < CHAR_BUFFER_SIZE - 1) {
                buffer_input[index++] = c;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

SemaphoreHandle_t luaMutex;

void app_main(void)
{
  // Init mutex
  luaMutex = xSemaphoreCreateMutex();

  _UartInit();
  
  L = luaL_newstate();
  luaL_openlibs(L);

  LittleFSInit_(L);
  
// Registrasi modul ke Lua
luaL_requiref(L, "gpio", LUA_OPEN_GPIO, 1); 
luaL_requiref(L, "fs", LUA_OPEN_FS, 1);
//luaL_requiref(L, "os", LUA_OPEN_OS, 1);
luaL_requiref(L, "task", LUA_OPEN_TASK, 1);
lua_pop(L, 1);

  xSemaphoreTake(luaMutex, portMAX_DELAY);

  if(luaL_dofile(L, "/home/boot.lua") != LUA_OK) 
  { 
    LOG_W("running boot.lua: %s\n", lua_tostring(L, -1));
    lua_pop(L, 1);
  }

  xSemaphoreGive(luaMutex);

  xTaskCreate(_ShellTask, "Shell Task", 2048, NULL, 5, NULL);
  xTaskCreate(_HandleLuaCommand, "Handle Lua Task", 4096, NULL, 5, &consumerTaskHandle);

}
