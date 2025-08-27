#include "fos_semaphore.h"

static int L_SEM_CREATE(lua_State *L) {
    int count = luaL_checkinteger(L, 1);
    lua_semaphore_t *s = malloc(sizeof(lua_semaphore_t));
    s->handle = xSemaphoreCreateCounting(count, 0);
    if (!s->handle) {
        free(s);
        lua_pushnil(L);
        return 1;
    }
    lua_pushlightuserdata(L, s);
    return 1;
}

static int L_SEM_TAKE(lua_State *L) {
    lua_semaphore_t *s = lua_touserdata(L, 1);
    xSemaphoreTake(s->handle, portMAX_DELAY);
    return 0;
}

static int L_SEM_GIVE(lua_State *L) {
    lua_semaphore_t *s = lua_touserdata(L, 1);
    xSemaphoreGive(s->handle);
    return 0;
}

static const luaL_Reg SEMAPHORE_LIB[] = {
    { "create",   L_SEM_CREATE },
    { "take",     L_SEM_TAKE },
    { "give",  L_SEM_GIVE },
    { NULL, NULL }
};

int LUA_OPEN_SEMAPHORE( lua_State *L ) { luaL_newlib(L, SEMAPHORE_LIB); return 1; }