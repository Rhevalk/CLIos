#include "task.h"

extern lua_State *L;       // lua_State utama
extern SemaphoreHandle_t luaMutex;

typedef struct {
    lua_State *L;
    int func_ref;
} TaskCtx;

static TaskHandle_t get_task_handle(lua_State *L, const char *name) {
    lua_getfield(L, LUA_REGISTRYINDEX, "task_registry");
    lua_getfield(L, -1, name);
    TaskHandle_t h = (TaskHandle_t)lua_touserdata(L, -1);
    lua_pop(L, 2);
    return h;
}

static void task_runner(void *pv) {
    TaskCtx *ctx = (TaskCtx *)pv;

    // Kunci akses ke Lua
    xSemaphoreTake(luaMutex, portMAX_DELAY);

    // Panggil fungsi Lua yang direferensikan
    lua_rawgeti(ctx->L, LUA_REGISTRYINDEX, ctx->func_ref); // push func
    if (lua_pcall(ctx->L, 0, 0, 0) != LUA_OK) {
        const char *err = lua_tostring(ctx->L, -1);
        printf("Lua task error: %s\n", err ? err : "(nil)");
        lua_pop(ctx->L, 1);
    }

    // Bersih-bersih
    luaL_unref(ctx->L, LUA_REGISTRYINDEX, ctx->func_ref);
    xSemaphoreGive(luaMutex);

    vPortFree(ctx);
    vTaskDelete(NULL);
}

static int L_TASK_CREATE(lua_State *L) {
    const char *taskName   = luaL_checkstring(L, 1);
    UBaseType_t stackDepth = luaL_checkinteger(L, 2);
    luaL_checktype(L, 3, LUA_TFUNCTION);

    // simpan fungsi Lua
    lua_pushvalue(L, 3);
    int func_ref = luaL_ref(L, LUA_REGISTRYINDEX);

    TaskCtx *ctx = pvPortMalloc(sizeof(TaskCtx));
    ctx->L = L;
    ctx->func_ref = func_ref;

    TaskHandle_t handle;
    if (xTaskCreate(task_runner, taskName, stackDepth, ctx,
                    tskIDLE_PRIORITY + 1, &handle) != pdPASS) {
        luaL_unref(L, LUA_REGISTRYINDEX, func_ref);
        vPortFree(ctx);
        return luaL_error(L, "xTaskCreate failed");
    }

    // simpan di registry Lua (global task_registry[taskName] = handle)
    lua_getfield(L, LUA_REGISTRYINDEX, "task_registry");
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        lua_newtable(L);
        lua_pushvalue(L, -1);
        lua_setfield(L, LUA_REGISTRYINDEX, "task_registry");
    }
    lua_pushlightuserdata(L, handle);
    lua_setfield(L, -2, taskName);
    lua_pop(L, 1);

    return 0;
}

static int L_TASK_SUSPEND(lua_State *L) {
    const char *name = luaL_checkstring(L, 1);
    TaskHandle_t h = get_task_handle(L, name);
    if (h) vTaskSuspend(h);
    return 0;
}

static int L_TASK_RESUME(lua_State *L) {
    const char *name = luaL_checkstring(L, 1);
    TaskHandle_t h = get_task_handle(L, name);
    if (h) vTaskResume(h);
    return 0;
}

static int L_TASK_DELETE(lua_State *L) {
    const char *name = luaL_checkstring(L, 1);
    TaskHandle_t h = get_task_handle(L, name);
    if (h) vTaskDelete(h);
    return 0;
}

static int L_TASK_DELAY(lua_State *L) {
  int ms = luaL_checkinteger(L, 1);
  vTaskDelay(pdMS_TO_TICKS(ms));
  return 0;
}

const luaL_Reg TASK_LIB[] = {
  {"create", L_TASK_CREATE},
  {"suspend", L_TASK_SUSPEND},
  {"resume", L_TASK_RESUME},
  {"delete", L_TASK_DELETE},
  {"delay", L_TASK_DELAY},
  {NULL, NULL}
};

int LUA_OPEN_TASK( lua_State *L ) { luaL_newlib(L, TASK_LIB); return 1; }


/*
local rtos = require("rtos")

-- bikin task
rtos.task_create("hello", 2048, function()
    while true do
        print("Hello from task")
        rtos.sleep(1000)
    end
end)

rtos.sleep(2000)
rtos.suspend("hello")
print("Task suspended")

rtos.sleep(2000)
rtos.resume("hello")
print("Task resumed")

rtos.sleep(5000)
rtos.delete("hello")
print("Task deleted")

Jadi intinya:

Bisa pakai object-oriented style (t:suspend()) → tiap task punya userdata sendiri.

Bisa juga pakai name-based functional style (rtos.suspend("hello")).

static int L_TASK_CREATE(lua_State *L) {
    const char *taskName   = luaL_checkstring(L, 1);
    UBaseType_t stackDepth = (UBaseType_t)luaL_checkinteger(L, 2); // biasanya dalam words
    luaL_checktype(L, 3, LUA_TFUNCTION);

    // Siapkan konteks untuk task
    TaskCtx *ctx = pvPortMalloc(sizeof(TaskCtx));
    if (!ctx) return luaL_error(L, "out of memory");
    ctx->L = L;

    // Ambil ref ke fungsi (push dulu nilainya ke top)
    lua_pushvalue(L, 3);
    ctx->func_ref = luaL_ref(L, LUA_REGISTRYINDEX);

    // Buat task
    BaseType_t rc = xTaskCreate(task_runner, taskName, stackDepth, ctx,
                                tskIDLE_PRIORITY + 1, NULL);
    if (rc != pdPASS) {
        luaL_unref(L, LUA_REGISTRYINDEX, ctx->func_ref);
        vPortFree(ctx);
        return luaL_error(L, "xTaskCreate failed");
    }

    return 0;
}

*/