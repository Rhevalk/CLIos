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

const char *SYSTEM = "SYS";
const char *USER = "USER";

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


char buffers[2][CHAR_BUFFER_SIZE];
atomic_bool active_index = false;
TaskHandle_t consumerTaskHandle;

static char lua_buffer[1024];     // buffer multiline
static size_t lua_buf_len = 0;    // panjang isi

void _HandleLuaCommand(void *pvParameters) {
  char local_buffer[CHAR_BUFFER_SIZE];

  while (1) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    bool index = atomic_load(&active_index);
    strncpy(local_buffer, buffers[index], CHAR_BUFFER_SIZE);

    // Tambah input ke buffer multiline
    size_t len = strlen(local_buffer);
    if (lua_buf_len + len + 2 >= sizeof(lua_buffer)) {
        ESP_LOGE(SYSTEM, "Input too long");
        lua_buf_len = 0;
        continue;
    }
    memcpy(lua_buffer + lua_buf_len, local_buffer, len);
    lua_buf_len += len;
    lua_buffer[lua_buf_len++] = '\n';
    lua_buffer[lua_buf_len] = '\0';

    // Bersihkan stack
    lua_settop(L, 0);
    lua_sethook(L, interrupt_hook, LUA_MASKCOUNT | LUA_MASKCALL | LUA_MASKRET, 10);

    // Coba compile dulu
    int status = luaL_loadbuffer(L, lua_buffer, lua_buf_len, "line");
    if (status == LUA_OK) {
        // ✅ kode lengkap
        status = lua_pcall(L, 0, LUA_MULTRET, 0);
        if (status != LUA_OK) {
            ESP_LOGE(SYSTEM, "Runtime error: %s", lua_tostring(L, -1));
            lua_pop(L, 1);

            atomic_store(&stop_flag, false); 
            continue;
        }
        lua_buf_len = 0;
        // ESP_LOGI(SYSTEM, ">");
    } else {
        const char *err = lua_tostring(L, -1);

        if (err && strstr(err, "near <eof>")) {
            // 🔍 kalau pesan error ada 'expected' juga → incomplete block
            if (strstr(err, "expected")) {
                ESP_LOGI(SYSTEM, "...");
                lua_pop(L, 1);
                continue; // tunggu baris berikutnya
            }

            // coba sekali lagi dengan newline tambahan
            lua_buffer[lua_buf_len++] = '\n';
            lua_buffer[lua_buf_len] = '\0';
            lua_settop(L, 0);
            int retry = luaL_loadbuffer(L, lua_buffer, lua_buf_len, "line");
            if (retry == LUA_OK) {
                ESP_LOGI(SYSTEM, "...");
                lua_pop(L, 1);
                continue;
            }
        }

        // ❌ error nyata
        ESP_LOGE(SYSTEM, "Syntax error: %s", err);
        lua_pop(L, 1);
        lua_buf_len = 0;
        // ESP_LOGI(SYSTEM, ">");
    }

    atomic_store(&stop_flag, false);
    lua_sethook(L, NULL, 0, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

// Task untuk CLI
void _ShellTask(void *pvParameters) {
    static char buffer[CHAR_BUFFER_SIZE]; // static
    static size_t index = 0; // static
    uint8_t data;

    while (1) 
    {
        int len = uart_read_bytes(UART_NUM_0, &data, 1, pdMS_TO_TICKS(10));
        if (len > 0) 
        {
          char c = (char)data;

            if (c == 'q') { 
                if(!atomic_load(&stop_flag)) {
                    atomic_store(&stop_flag, true);
                    ESP_LOGI(SYSTEM, "Interrupt signal sent to Lua");
                }
                continue;
            }

            if (c == '\r' || c == '\n') 
            {
                buffer[index] = '\0';
                if (index > 0) 
                {
                  bool next_index = !atomic_load(&active_index);

                  strncpy(buffers[next_index], buffer, CHAR_BUFFER_SIZE);
                  // swap buffer atomik
                  atomic_store(&active_index, next_index);
                  // notifikasi ke consumer
                  xTaskNotifyGive(consumerTaskHandle);

                  ESP_LOGI(USER, "%s", buffer); // Log input
                  //_HandleLuaCommand(buffer); // Handle root commands
                  index = 0;
                }
            } 

            else if (index < sizeof(buffer) - 1)
              buffer[index++] = c;
        }
        // printf("Free stack Task CLI: %d\n", uxTaskGetStackHighWaterMark(NULL));
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

SemaphoreHandle_t luaMutex;

void app_main(void)
{
  _UartInit();

    // Init mutex
  luaMutex = xSemaphoreCreateMutex();
  
  L = luaL_newstate();
  luaL_openlibs(L);

  LittleFSInit_(L);
  
	// Registrasi modul ke Lua
	luaL_requiref(L, "gpio", LUA_OPEN_GPIO, 1);
	lua_pop(L, 1); 
	
	luaL_requiref(L, "fs", LUA_OPEN_FS, 1);
	lua_pop(L, 1);
	
	//luaL_requiref(L, "os", LUA_OPEN_OS, 1);
	//lua_pop(L, 1);

  luaL_requiref(L, "task", LUA_OPEN_TASK, 1);
	lua_pop(L, 1);

  xSemaphoreTake(luaMutex, portMAX_DELAY);
  if(luaL_dofile(L, "/home/boot.lua") != LUA_OK) { 
      printf("running boot.lua: %s\n", lua_tostring(L, -1));
      lua_pop(L, 1);
  }
  xSemaphoreGive(luaMutex);

  xTaskCreate(_ShellTask, "Shell Task", 2048, NULL, 5, NULL);
  xTaskCreate(_HandleLuaCommand, "Handle Lua Task", 4096, NULL, 5, &consumerTaskHandle);

}
