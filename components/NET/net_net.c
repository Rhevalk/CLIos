// ================================ net ====================================
// net.init() → initialize NVS, esp_netif, event loop (safe to call multiple times)
// net.time() → esp_timer_get_time()/1000000
// net.hostname([name]) -> set/get hostname (default interface)

#include "net_net.h"

static bool g_net_inited=false;

static int L_NET_INIT(lua_State *L){
    if(!g_net_inited){
        esp_err_t err;
        // NVS is needed by Wi‑Fi
        err = nvs_flash_init();
        if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND){
            ESP_ERROR_CHECK(nvs_flash_erase());
            ESP_ERROR_CHECK(nvs_flash_init());
        }
        ESP_ERROR_CHECK(esp_netif_init());
        ESP_ERROR_CHECK(esp_event_loop_create_default());
        g_net_inited=true;
    }
    lua_pushboolean(L,1); return 1;
}

static int L_NET_TIME(lua_State *L){
    int64_t us = esp_timer_get_time();
    lua_pushnumber(L, (lua_Number)(us/1000000.0));
    return 1;
}

static int L_NET_HOSTNAME(lua_State *L){
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if(!netif) netif = esp_netif_get_handle_from_ifkey("ETH_DEF");
    if(!netif){ lua_pushnil(L); return 1; }
    if(lua_gettop(L)>=1 && !lua_isnil(L,1)){
        const char* h = luaL_checkstring(L,1);
        ESP_ERROR_CHECK(esp_netif_set_hostname(netif, h));
    }
    const char* name = NULL; ESP_ERROR_CHECK(esp_netif_get_hostname(netif, &name));
    lua_pushstring(L, name?name:"");
    return 1;
}

static const luaL_Reg NET_LIB[]={
    {"init", L_NET_INIT},
    {"time", L_NET_TIME},
    {"hostname", L_NET_HOSTNAME},
    {NULL,NULL}
};

int LUA_OPEN_NET(lua_State *L) { luaL_newlib(L, NET_LIB); return 1; }