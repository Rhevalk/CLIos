#include "fos_mutex.h"

static int L_MUTEX_CREATE(lua_State *L) {
    lua_mutex_t *m = malloc(sizeof(lua_mutex_t));
    m->handle = xSemaphoreCreateMutex();
    if (!m->handle) {
        free(m);
        lua_pushnil(L);
        return 1;
    }
    lua_pushlightuserdata(L, m);
    return 1;
}

static int L_MUTEX_LOCK(lua_State *L) {
    lua_mutex_t *m = lua_touserdata(L, 1);
    xSemaphoreTake(m->handle, portMAX_DELAY);
    return 0;
}

static int L_MUTEX_UNLOCK(lua_State *L) {
    lua_mutex_t *m = lua_touserdata(L, 1);
    xSemaphoreGive(m->handle);
    return 0;
}

static const luaL_Reg MUTEX_LIB[] = {
    { "create",   L_MUTEX_CREATE},
    { "lock",     L_MUTEX_LOCK },
    { "unlock",  L_MUTEX_UNLOCK },
    { NULL, NULL }
};


int LUA_OPEN_MUTEX( lua_State *L ) { luaL_newlib(L, MUTEX_LIB); return 1; }
