#include "fos_queue.h"

// Buat queue
static int L_QUEUE_CREATE(lua_State *L) {
    int length = luaL_checkinteger(L, 1);
    lua_queue_t *q = malloc(sizeof(lua_queue_t));
    q->handle = xQueueCreate(length, sizeof(int)); // contoh int, bisa diubah
    if (!q->handle) {
        free(q);
        lua_pushnil(L);
        return 1;
    }
    lua_pushlightuserdata(L, q);
    return 1;
}

// Kirim data ke queue
static int L_QUEUE_SEND(lua_State *L) {
    lua_queue_t *q = lua_touserdata(L, 1);
    int val = luaL_checkinteger(L, 2);
    xQueueSend(q->handle, &val, portMAX_DELAY);
    return 0;
}

// Ambil data dari queue
static int L_QUEUE_RECEIVE(lua_State *L) {
    lua_queue_t *q = lua_touserdata(L, 1);
    int val;
    xQueueReceive(q->handle, &val, portMAX_DELAY);
    lua_pushinteger(L, val);
    return 1;
}


static const luaL_Reg QUEUE_LIB[] = {
    { "create",   L_QUEUE_CREATE },
    { "send",     L_QUEUE_SEND },
    { "receive",  L_QUEUE_RECEIVE },
    { NULL, NULL }
};


int LUA_OPEN_QUEUE( lua_State *L ) { luaL_newlib(L, QUEUE_LIB); return 1; }