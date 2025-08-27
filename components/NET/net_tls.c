#include "net_tls.h"
#include "esp_tls.h"
#include "lua.h"
#include "lauxlib.h"

// ================================ TLS (esp_tls) ==========================
// tls.connect(host, port, [alpn]) -> handle ; :write(s) :read(n) :close()

// Forward declaration metode Lua
static int F_TLS_WRITE(lua_State *L);
static int F_TLS_READ(lua_State *L);
static int F_TLS_CLOSE(lua_State *L);

static int L_TLS_CONNECT(lua_State *L){
    const char* host = luaL_checkstring(L,1);
    int port = luaL_optinteger(L,2,443);
    const char* alpn = luaL_optstring(L,3,NULL);

    esp_tls_cfg_t cfg;
    memset(&cfg,0,sizeof(cfg));
    cfg.cacert_pem_buf = NULL;
    cfg.cacert_pem_bytes = 0;
    cfg.clientcert_pem_buf = NULL;
    cfg.clientcert_pem_bytes = 0;
    cfg.clientkey_pem_buf = NULL;
    cfg.clientkey_pem_bytes = 0;
    cfg.non_block = false;
    cfg.timeout_ms = 0;
    cfg.alpn_protos = NULL;

    if(alpn){
        static const char* alpn_list[2];
        alpn_list[0] = alpn;
        alpn_list[1] = NULL;
        cfg.alpn_protos = alpn_list;
    }

    // pointer TLS
    esp_tls_t *tls = NULL;

    esp_err_t err = esp_tls_conn_new_sync(host, strlen(host), port, &cfg, tls);
    if(err != ESP_OK){
        return luaL_error(L,"TLS connect failed: %d", err);
    }

    lua_tls_t *h = (lua_tls_t*)lua_newuserdata(L,sizeof(*h));
    h->tls = tls;

    if(luaL_newmetatable(L,"tls.handle")){
        luaL_Reg mt[] = {
            {"write", F_TLS_WRITE},
            {"read", F_TLS_READ},
            {"close", F_TLS_CLOSE},
            {NULL,NULL}
        };
        luaL_setfuncs(L, mt, 0);
        lua_pushvalue(L,-1);
        lua_setfield(L,-2,"__index");
    }

    lua_setmetatable(L,-2);
    return 1;
}


// ================================ METHODS ================================
static int F_TLS_WRITE(lua_State *L){
    lua_tls_t *h = (lua_tls_t*)luaL_checkudata(L,1,"tls.handle");
    size_t n;
    const char* buf = luaL_checklstring(L,2,&n);
    int w = esp_tls_conn_write(h->tls, buf, n);
    lua_pushinteger(L,w);
    return 1;
}

static int F_TLS_READ(lua_State *L){
    lua_tls_t *h = (lua_tls_t*)luaL_checkudata(L,1,"tls.handle");
    int n = luaL_checkinteger(L,2);
    char* buf = (char*)malloc(n);
    if(!buf) return luaL_error(L,"out of memory");
    int r = esp_tls_conn_read(h->tls, buf, n);
    lua_pushlstring(L, buf, (r>0)?r:0);
    free(buf);
    return 1;
}

static int F_TLS_CLOSE(lua_State *L){
    lua_tls_t *h = (lua_tls_t*)luaL_checkudata(L,1,"tls.handle");
    if(h->tls){
        esp_tls_conn_destroy(h->tls); // ESP-IDF v4.x
        free(h->tls);
        h->tls = NULL;
    }
    lua_pushboolean(L,1);
    return 1;
}

// ================================ LIBRARY ================================
static const luaL_Reg TLS_LIB[] = {
    {"connect", L_TLS_CONNECT},
    {NULL,NULL}
};

int LUA_OPEN_TLS(lua_State *L){
    luaL_newlib(L, TLS_LIB);
    return 1;
}
