#include "task.h"

// Fungsi task FreeRTOS yang memanggil fungsi Lua
// Task worker → cuma jalan kalau ada pesan
static void lua_task(void *pvParameters) {
    lua_task_param_t *param = (lua_task_param_t *)pvParameters;
    lua_State *L = param->L;

    int msg;
    while (!param->stop_flag) {
        // Tunggu trigger (blocking)
        if (xQueueReceive(param->queue, &msg, portMAX_DELAY) == pdTRUE) {
            lua_rawgeti(L, LUA_REGISTRYINDEX, param->func_ref);
            if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
                LOG_E("Lua task error: %s\n", lua_tostring(L, -1));
                lua_pop(L, 1);
            }
        }
    }

    // Bersih-bersih
    luaL_unref(L, LUA_REGISTRYINDEX, param->func_ref);
    vQueueDelete(param->queue);
    free(param);
    vTaskDelete(NULL);
}


static int L_TASK_CREATE(lua_State *L) {
    luaL_checktype(L, 1, LUA_TFUNCTION);

    int stack_size = luaL_optinteger(L, 2, 2048);
    uint8_t priority = luaL_optinteger(L, 3, 1);
    const char* name = luaL_optstring(L, 4, "Task");

    lua_pushvalue(L, 1);
    int func_ref = luaL_ref(L, LUA_REGISTRYINDEX);

    lua_task_param_t *param = malloc(sizeof(lua_task_param_t));
    param->L = L;
    param->func_ref = func_ref;
    param->stop_flag = false;
    param->queue = xQueueCreate(10, sizeof(int)); // queue untuk trigger

    TaskHandle_t task_handle;
    BaseType_t res = xTaskCreate(
        lua_task,
        name,
        stack_size / sizeof(StackType_t),
        param,
        priority,
        &task_handle
    );
    param->task_handle = task_handle;

    if (res != pdPASS) {
        vQueueDelete(param->queue);
        free(param);
        lua_pushnil(L);
    } else {
        lua_pushlightuserdata(L, param); // handle task
    }

    return 1;
}


// Fungsi untuk trigger eksekusi task Lua
static int L_TASK_START(lua_State *L) {
    lua_task_param_t *param = lua_touserdata(L, 1);
    if (!param || !param->queue) return 0;

    int dummy = 1;
    xQueueSend(param->queue, &dummy, 0);

    return 0;
}

static int L_TASK_STOP(lua_State *L) {
    lua_task_param_t *param = lua_touserdata(L, 1);
    if (param) param->stop_flag = true;
    return 0;
}

static int L_TASK_SUSPEND(lua_State *L) {
    lua_task_param_t *param = lua_touserdata(L, 1);
    if (param && param->task_handle) vTaskSuspend(param->task_handle);
    return 0;
}

static int L_TASK_RESUME(lua_State *L) {
    lua_task_param_t *param = lua_touserdata(L, 1);
    if (param && param->task_handle) vTaskResume(param->task_handle);
    return 0;
}


static int L_TASK_DELAY(lua_State *L) {
  int ms = luaL_checkinteger(L, 1);
  vTaskDelay(pdMS_TO_TICKS(ms));
  return 0;
}

static const luaL_Reg TASK_LIB[] = {
    { "create",   L_TASK_CREATE },
    { "stop",     L_TASK_STOP },
    { "start",     L_TASK_START },
    { "suspend",  L_TASK_SUSPEND },
    { "resume",   L_TASK_RESUME },
    { "delay",    L_TASK_DELAY },
    { NULL, NULL }
};


int LUA_OPEN_TASK( lua_State *L ) { luaL_newlib(L, TASK_LIB); return 1; }