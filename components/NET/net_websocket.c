#include "net_websocket.h"

static void ensure_ok(lua_State *L, esp_err_t err){
    if(err != ESP_OK) luaL_error(L,"esp_err=%d",(int)err);
}

// ===== FUNGSI METODE WS =====
static int ws_send(lua_State *L){
    lua_ws_t *h = (lua_ws_t*)luaL_checkudata(L,1,"ws.handle");
    size_t n;
    const char *msg = luaL_checklstring(L,2,&n);
    int r = esp_websocket_client_send_text(h->c,msg,n,portMAX_DELAY);
    lua_pushinteger(L,r);
    return 1;
}

static int ws_close(lua_State *L){
    lua_ws_t *h = (lua_ws_t*)luaL_checkudata(L,1,"ws.handle");
    if(h->c){
        esp_websocket_client_stop(h->c);
        esp_websocket_client_destroy(h->c);
        h->c = NULL;
    }
    lua_pushboolean(L,1);
    return 1;
}

// ===== WS CONNECT =====
static int L_WS_CONNECT(lua_State *L){
    const char *url = luaL_checkstring(L,1);
    esp_websocket_client_config_t cfg = { .uri = url };

    lua_ws_t *h = (lua_ws_t*)lua_newuserdata(L,sizeof(*h));
    h->c = esp_websocket_client_init(&cfg);
    ensure_ok(L, esp_websocket_client_start(h->c));

    if(luaL_newmetatable(L,"ws.handle")){
        luaL_Reg mt[] = {
            {"send", ws_send},
            {"close", ws_close},
            {NULL,NULL}
        };
        luaL_setfuncs(L, mt, 0);
        lua_pushvalue(L,-1);
        lua_setfield(L,-2,"__index");
    }

    lua_setmetatable(L,-2);
    return 1;
}

static const luaL_Reg WS_LIB[] = {
    {"connect", L_WS_CONNECT},
    {NULL,NULL}
};

int LUA_OPEN_WEBSOCKET(lua_State *L){
    luaL_newlib(L, WS_LIB);
    return 1;
}
