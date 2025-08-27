#include "net_httpd.h"

// ===================== Lua HTTP Server =====================

// Registry key untuk table callback
#define LUA_HTTPD_CB_TABLE "HTTPD_CALLBACKS"

// ------------------- Helper -------------------
// Store Lua callback per path+method
static void store_lua_callback(lua_State *L, const char* path, const char* method, int func_ref){
    lua_getfield(L, LUA_REGISTRYINDEX, LUA_HTTPD_CB_TABLE);
    if(lua_isnil(L,-1)){
        lua_pop(L,1);
        lua_newtable(L);
        lua_pushvalue(L,-1);
        lua_setfield(L,LUA_REGISTRYINDEX,LUA_HTTPD_CB_TABLE);
    }
    char key[128];
    snprintf(key,sizeof(key), "%s|%s", path, method);
    lua_rawseti(L,-1,func_ref);  // store function at integer index (simplified)
    lua_pop(L,1);
}

// ------------------- Generic Handler -------------------
static esp_err_t lua_httpd_generic_handler(httpd_req_t *req){
    lua_httpd_t *h = (lua_httpd_t*)req->user_ctx;
    lua_State *L = h->L;

    // Lookup callback
    char key[1024]; // buffer lebih besar
    snprintf(key, sizeof(key), "%s|%s", req->uri,
             req->method == HTTP_POST ? "POST" : "GET");

    lua_getfield(L,LUA_REGISTRYINDEX,LUA_HTTPD_CB_TABLE);
    if(lua_isnil(L,-1)){
        lua_pop(L,1);
        return httpd_resp_send_404(req);
    }

    lua_getfield(L,-1,key);
    if(!lua_isfunction(L,-1)){
        lua_pop(L,2);
        return httpd_resp_send_404(req);
    }

    // panggil callback Lua
    lua_pushstring(L, req->uri);
    if(lua_pcall(L,1,2,0)!=LUA_OK){
        const char* err = lua_tostring(L,-1);
        (void)err; // agar warning hilang
        lua_pop(L,1);
        return httpd_resp_send_500(req);
    }

    int status = luaL_checkinteger(L,-2);
    (void)status; // jika status tidak dipakai
    size_t len;
    const char* body = luaL_checklstring(L,-1,&len);

    httpd_resp_set_status(req,"200 OK");
    httpd_resp_send(req, body, len);

    lua_pop(L,2);
    return ESP_OK;
}


// ------------------- Lua methods -------------------
static int F_STOP(lua_State *L){
    lua_httpd_t *h = (lua_httpd_t*)luaL_checkudata(L,1,"httpd.handle");
    if(h->srv) httpd_stop(h->srv);
    h->srv=NULL;
    lua_pushboolean(L,1);
    return 1;
}

static int F_ON(lua_State *L){
    lua_httpd_t *h = (lua_httpd_t*)luaL_checkudata(L,1,"httpd.handle");
    const char* path = luaL_checkstring(L,2);
    const char* method = luaL_optstring(L,3,"GET");
    luaL_checktype(L,4,LUA_TFUNCTION);

    lua_pushvalue(L,4);
    int ref = luaL_ref(L,LUA_REGISTRYINDEX);
    store_lua_callback(L, path, method, ref);

    httpd_uri_t uri = {
        .uri = path,
        .method = (!strcasecmp(method,"POST")?HTTP_POST:HTTP_GET),
        .handler = lua_httpd_generic_handler,
        .user_ctx = h
    };
    return httpd_register_uri_handler(h->srv,&uri)==ESP_OK ?
           (lua_pushboolean(L,1),1) : (lua_pushboolean(L,0),1);
}

// ------------------- HTTPD_START -------------------
static int L_HTTPD_START(lua_State *L){
    int port = luaL_optinteger(L,1,80);

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = port;
    config.lru_purge_enable = true;

    lua_httpd_t *h = (lua_httpd_t*)lua_newuserdata(L,sizeof(*h));
    h->srv = NULL;
    h->L = L;

    if(httpd_start(&h->srv, &config)!=ESP_OK)
        return luaL_error(L,"httpd_start failed");

    if(luaL_newmetatable(L,"httpd.handle")){
        luaL_Reg mt[] = {{"stop",F_STOP},{"on",F_ON},{NULL,NULL}};
        luaL_setfuncs(L,mt,0);
        lua_pushvalue(L,-1);
        lua_setfield(L,-2,"__index");
    }
    lua_setmetatable(L,-2);
    return 1;
}

// ------------------- Lua Library -------------------
static const luaL_Reg HTTPD_LIB[] = {
    {"start",L_HTTPD_START},
    {NULL,NULL}
};

int LUA_OPEN_HTTPD(lua_State *L){
    luaL_newlib(L,HTTPD_LIB);
    return 1;
}
