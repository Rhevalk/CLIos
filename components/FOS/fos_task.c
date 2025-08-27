// ============================= IMPLEMENTATION =============================
//
// t = task.create(function()
//     print("running...")
// end, 2048, 1, "MyTask", 500)
// 
// task.start(t)
// task.suspend(t)
// task.resume(t)
// task.stop(t)
//
// -- OR
//
// t:start()
// t:suspend()
// t:resume()
// t:stop()
// 
// task.delay(1000)  -- delay 1 detik
// 
// ============================= IMPLEMENTATION =============================

#include "fos_task.h"

extern SemaphoreHandle_t luaMutex;

// ------------------------- Worker Task --------------------------
static void lua_task(void *pvParameters) {
    lua_task_param_t *param = (lua_task_param_t *)pvParameters;
    lua_State *L = param->L;

    int msg;
    // Tunggu trigger (blocking)
    if (xQueueReceive(param->queue, &msg, portMAX_DELAY) == pdTRUE) {
        TickType_t xLastWakeTime = xTaskGetTickCount();
        const TickType_t xInterval = param->timer / portTICK_PERIOD_MS;
        while (!param->stop_flag) {

            xSemaphoreTake(luaMutex, portMAX_DELAY);
            lua_rawgeti(L, LUA_REGISTRYINDEX, param->func_ref);
            if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
                LOG_E("Lua task error: %s\n", lua_tostring(L, -1));
                lua_pop(L, 1);
            }
            xSemaphoreGive(luaMutex);

            vTaskDelayUntil(&xLastWakeTime, xInterval);
        }
    }

    // Bersih-bersih
    luaL_unref(L, LUA_REGISTRYINDEX, param->func_ref);
    vQueueDelete(param->queue);
    free(param);
    vTaskDelete(NULL);
}

// ------------------------- Helper --------------------------
static lua_task_param_t* check_task(lua_State *L, int idx) {
    lua_task_param_t **ud = (lua_task_param_t **)luaL_checkudata(L, idx, "TaskMT");
    return *ud;
}

// ------------------------- Lua Bindings --------------------------
static int L_TASK_CREATE(lua_State *L) {
    luaL_checktype(L, 1, LUA_TFUNCTION);

    int stack_size = luaL_optinteger(L, 2, 2048);
    uint8_t priority = luaL_optinteger(L, 3, 1);
    const char* name = luaL_optstring(L, 4, "Task");
    int _timer = luaL_optinteger(L, 5, 100);
    if (_timer == 0) _timer = 100;

    lua_pushvalue(L, 1);
    int func_ref = luaL_ref(L, LUA_REGISTRYINDEX);

    lua_task_param_t *param = malloc(sizeof(lua_task_param_t));
    param->L = L;
    param->func_ref = func_ref;
    param->stop_flag = false;
    param->queue = xQueueCreate(10, sizeof(int));
    param->timer = _timer;

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
        // Buat userdata
        lua_task_param_t **ud = (lua_task_param_t **)lua_newuserdata(L, sizeof(lua_task_param_t *));
        *ud = param;

        // Set metatable
        luaL_getmetatable(L, "TaskMT");
        lua_setmetatable(L, -2);
    }

    return 1;
}

static int L_TASK_START(lua_State *L) {
    lua_task_param_t *param = check_task(L, 1);
    if (param && param->queue) {
        int dummy = 1;
        xQueueSend(param->queue, &dummy, 0);
    }
    return 0;
}

static int L_TASK_STOP(lua_State *L) {
    lua_task_param_t *param = check_task(L, 1);
    if (param) param->stop_flag = true;
    return 0;
}

static int L_TASK_SUSPEND(lua_State *L) {
    lua_task_param_t *param = check_task(L, 1);
    if (param && param->task_handle) vTaskSuspend(param->task_handle);
    return 0;
}

static int L_TASK_RESUME(lua_State *L) {
    lua_task_param_t *param = check_task(L, 1);
    if (param && param->task_handle) vTaskResume(param->task_handle);
    return 0;
}

static int L_TASK_DELAY(lua_State *L) {
    int ms = luaL_checkinteger(L, 1);
    vTaskDelay(pdMS_TO_TICKS(ms));
    return 0;
}

// ------------------------- GC Cleanup --------------------------
static int L_TASK_GC(lua_State *L) {
    lua_task_param_t *param = check_task(L, 1);
    if (param) {
        param->stop_flag = true;
        if (param->queue) vQueueDelete(param->queue);
        luaL_unref(L, LUA_REGISTRYINDEX, param->func_ref);
        free(param);
    }
    return 0;
}

// ------------------------- Registrasi --------------------------
static const luaL_Reg TASK_LIB[] = {
    { "create",  L_TASK_CREATE },
    { "start",   L_TASK_START },
    { "stop",    L_TASK_STOP },
    { "suspend", L_TASK_SUSPEND },
    { "resume",  L_TASK_RESUME },
    { "delay",   L_TASK_DELAY },
    { NULL, NULL }
};

static const luaL_Reg TASK_METHODS[] = {
    { "start",   L_TASK_START },
    { "stop",    L_TASK_STOP },
    { "suspend", L_TASK_SUSPEND },
    { "resume",  L_TASK_RESUME },
    { "__gc",    L_TASK_GC },
    { NULL, NULL }
};

int LUA_OPEN_TASK(lua_State *L) {
    // Register global lib
    luaL_newlib(L, TASK_LIB);

    // Buat metatable untuk Task
    if (luaL_newmetatable(L, "TaskMT")) {
        lua_pushvalue(L, -1);
        lua_setfield(L, -2, "__index"); // mt.__index = mt
        luaL_setfuncs(L, TASK_METHODS, 0);
    }
    lua_pop(L, 1); // pop metatable

    return 1;
}
