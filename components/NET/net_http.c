// ================================ HTTP Client ============================
// http.request(method, url, body?, headers_table?) -> status, body

#include "net_http.h"

static void ensure_ok(lua_State *L, esp_err_t err){ if(err!=ESP_OK) luaL_error(L, "esp_err=%d", (int)err); }

static esp_err_t http_ev(esp_http_client_event_t *evt){
    return ESP_OK; // we use blocking perform + fetch body via client read
}

static int L_HTTP_REQUEST(lua_State *L){
    const char* method = luaL_optstring(L,1, "GET");
    const char* url    = luaL_checkstring(L,2);
    size_t blen=0; const char* body = luaL_optlstring(L,3, NULL, &blen);
    esp_http_client_config_t c={ .url=url, .event_handler=http_ev };
    esp_http_client_handle_t h=esp_http_client_init(&c);
    esp_http_client_set_method(h, (!strcasecmp(method,"POST")?HTTP_METHOD_POST:!strcasecmp(method,"PUT")?HTTP_METHOD_PUT:!strcasecmp(method,"DELETE")?HTTP_METHOD_DELETE:HTTP_METHOD_GET));
    if(body) esp_http_client_set_post_field(h, body, blen);
    if(lua_istable(L,4)){
        lua_pushnil(L); while(lua_next(L,4)!=0){ const char*k=luaL_checkstring(L,-2); const char*v=luaL_checkstring(L,-1); esp_http_client_set_header(h,k,v); lua_pop(L,1);} }
    ensure_ok(L, esp_http_client_perform(h));
    int status = esp_http_client_get_status_code(h);
    int content_len = esp_http_client_get_content_length(h);
    char *buf=NULL; int total=0;
    if(content_len>0){ buf=(char*)malloc(content_len+1); int r; while((r=esp_http_client_read(h, buf+total, content_len-total))>0) total+=r; buf[total]=0; }
    esp_http_client_cleanup(h);
    lua_pushinteger(L,status); lua_pushlstring(L, buf?buf: "", buf?total:0); if(buf) free(buf); return 2;
}

static const luaL_Reg HTTP_LIB[]={{"request",L_HTTP_REQUEST},{NULL,NULL}};
int LUA_OPEN_HTTP(lua_State *L){ luaL_newlib(L, HTTP_LIB); return 1; }