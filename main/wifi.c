#include "wifi.h"

bool wifi_init = false;

static esp_netif_t *sta_netif = NULL;
static esp_netif_t *ap_netif  = NULL;

static void wifi_event_handler(void *arg, esp_event_base_t base, int32_t id, void *data) {
    if (base == WIFI_EVENT) {
        switch (id) {
            case WIFI_EVENT_STA_START:    LOG_I("STA start"); break;
            case WIFI_EVENT_STA_CONNECTED:LOG_I("STA connected"); break;
            case WIFI_EVENT_STA_DISCONNECTED: {
                wifi_event_sta_disconnected_t *e = (wifi_event_sta_disconnected_t*)data;
                LOG_W("STA disconnected, reason=%d", e->reason);
                // auto-reconnect contoh (opsional):
                esp_wifi_connect();
            } break;
            case WIFI_EVENT_AP_START:     LOG_I("AP start"); break;
            case WIFI_EVENT_AP_STACONNECTED: {
                wifi_event_ap_staconnected_t *e = (wifi_event_ap_staconnected_t*)data;
                LOG_I("STA joined: " MACSTR, MAC2STR(e->mac));
            } break;
        }
    } else if (base == IP_EVENT) {
        if (id == IP_EVENT_STA_GOT_IP) {
            ip_event_got_ip_t *e = (ip_event_got_ip_t*)data;
            LOG_I("Got IP: " IPSTR, IP2STR(&e->ip_info.ip));
        }
    }
}

static void _WifiInitCore(void)
{
    // NVS wajib utk menyimpan kalibrasi Wi-Fi & kredensial
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(esp_netif_init());

    // Daftarkan handler event Wi-Fi & IP
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT,   ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
}

static void wifi_deinit(void) {
  if (wifi_init) {
    esp_wifi_stop();
    esp_wifi_deinit();
    
    // (opsional) hancurkan netif untuk benar2 bersih:
    if (sta_netif) { esp_netif_destroy_default_wifi(sta_netif); sta_netif=NULL; }
    if (ap_netif)  { esp_netif_destroy_default_wifi(ap_netif);  ap_netif=NULL; }
    
    wifi_init = false;
  }
}


static int L_WIFI_MODE(lua_State *L) {
    uint8_t mode = luaL_optinteger(L, 1, 0);

    if (!wifi_init) {
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));
        wifi_init = true;
    }

    if (mode == 0 && !sta_netif) sta_netif = esp_netif_create_default_wifi_sta();
    if (mode == 1 && !ap_netif)  ap_netif  = esp_netif_create_default_wifi_ap();
    if (mode == 2) {
        if (!sta_netif) sta_netif = esp_netif_create_default_wifi_sta();
        if (!ap_netif)  ap_netif  = esp_netif_create_default_wifi_ap();
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(mode));
    ESP_ERROR_CHECK(esp_wifi_start());
    return 0;
}

// ----------STA----------
static int L_WIFI_STA_START(lua_State *L) {
    const char *ssid = luaL_checkstring(L, 1);
    const char *pass = luaL_optstring(L, 2, "");

    wifi_config_t cfg = {0};
    strncpy((char*)cfg.sta.ssid, ssid, sizeof(cfg.sta.ssid)-1);
    strncpy((char*)cfg.sta.password, pass, sizeof(cfg.sta.password)-1);

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &cfg));
    ESP_ERROR_CHECK(esp_wifi_connect());
    return 0;
}

static int L_WIFI_STA_STOP(lua_State *L) {
    uint8_t _level = luaL_optinteger(L, 1, 0);

    if (!wifi_init) {
        // versi ringan: cuma push warning ke Lua, tidak pakai LOG
        lua_pushstring(L, "WiFi not initialized");
        lua_error(L);
        return 0; // tidak akan tercapai, lua_error longjmp
    }

    if (_level == 0) {
        ESP_ERROR_CHECK(esp_wifi_disconnect());
        ESP_ERROR_CHECK(esp_wifi_stop());
        // bisa push status ke Lua kalau mau
        lua_pushstring(L, "STA stopped");
        return 1;
    } 
    else if (_level == 1){
        if (sta_netif) { esp_netif_destroy_default_wifi(sta_netif); sta_netif=NULL; }
        wifi_deinit(); // hentikan WiFi & hapus AP jika ada
        lua_pushstring(L, "STA deleted");
        return 1;
    }
    else {
        lua_pushstring(L, "Invalid level: use 0=stop, 1=delete");
        lua_error(L);
        return 0; // tidak akan tercapai
    }
}


static int L_WIFI_STA_IP(lua_State *L) {
    if (sta_netif) {
        esp_netif_ip_info_t ip;
        if (esp_netif_get_ip_info(sta_netif, &ip) == ESP_OK) {
            char ip_str[16];
            snprintf(ip_str, sizeof(ip_str), IPSTR, IP2STR(&ip.ip));
            lua_pushstring(L, ip_str);
            return 1;
        }
    }
    lua_pushnil(L);
    return 1;
}


// ----------AP-----------
static int L_WIFI_AP_START(lua_State *L) {
    const char *ssid = luaL_checkstring(L, 1);
    const char *pass = luaL_optstring(L, 2, "");
    int channel = luaL_optinteger(L, 3, 1);

    wifi_config_t cfg = {0};
    strncpy((char*)cfg.ap.ssid, ssid, sizeof(cfg.ap.ssid)-1);
    cfg.ap.ssid_len = strlen(ssid);
    cfg.ap.channel = channel;
    cfg.ap.authmode = (strlen(pass) >= 8) ? WIFI_AUTH_WPA2_PSK : WIFI_AUTH_OPEN;
    if (strlen(pass) >= 8) strncpy((char*)cfg.ap.password, pass, sizeof(cfg.ap.password)-1);

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &cfg));
    return 0;
}

static int L_WIFI_AP_STOP(lua_State *L) {
    uint8_t _level = luaL_optinteger(L, 1, 0);

    if (!wifi_init) {
        // versi ringan: cuma push warning ke Lua, tidak pakai LOG
        lua_pushstring(L, "WiFi not initialized");
        lua_error(L);
        return 0; // tidak akan tercapai, lua_error longjmp
    }

    if (_level == 0) {
        ESP_ERROR_CHECK(esp_wifi_stop());
        // bisa push status ke Lua kalau mau
        lua_pushstring(L, "AP stopped");
        return 1;
    } 
    else if (_level == 1){
        if (ap_netif) { esp_netif_destroy_default_wifi(sta_netif); sta_netif=NULL; }
        wifi_deinit(); // hentikan WiFi & hapus AP jika ada
        lua_pushstring(L, "AP deleted");
        return 1;
    }
    else {
        lua_pushstring(L, "Invalid level: use 0=stop, 1=delete");
        lua_error(L);
        return 0; // tidak akan tercapai
    }
}


static int L_WIFI_AP_IP(lua_State *L) {
    if (ap_netif) {
        esp_netif_ip_info_t ip;
        if (esp_netif_get_ip_info(ap_netif, &ip) == ESP_OK) {
            char ip_str[16];
            snprintf(ip_str, sizeof(ip_str), IPSTR, IP2STR(&ip.ip));
            lua_pushstring(L, ip_str);
            return 1;
        }
    }
    lua_pushnil(L);
    return 1;
}


// wifi
static int L_WIFI_STATUS(lua_State *L) {
    wifi_ap_record_t ap_info;
    esp_err_t err = esp_wifi_sta_get_ap_info(&ap_info);

    if (err == ESP_OK) {
        lua_pushboolean(L, 1);   // true
    } else {
        lua_pushboolean(L, 0);   // false
    }

    return 1;
}

static const luaL_Reg WIFI_LIB[] = {
    { "mode",   L_WIFI_MODE },
    { "status", L_WIFI_STATUS },
    { NULL, NULL }
};

static const luaL_Reg WIFI_STA_LIB[] = {
    { "start",   L_WIFI_STA_START },
    { "stop",    L_WIFI_STA_STOP },
    { "ip",      L_WIFI_STA_IP },
    { NULL, NULL }
};

static const luaL_Reg WIFI_AP_LIB[] = {
    { "start",   L_WIFI_AP_START },
    { "stop",    L_WIFI_AP_STOP },
    { "ip",      L_WIFI_AP_IP },
    { NULL, NULL }
};


int LUA_OPEN_WIFI(lua_State *L) {

    _WifiInitCore();

    // 1. Wifi 
    luaL_newlib(L, WIFI_LIB); 

    // 2. Buat sub-table wifi.sta
    luaL_newlib(L, WIFI_STA_LIB);  
    lua_setfield(L, -2, "sta");   

    // 3. Buat sub-table wifi.ap
    luaL_newlib(L, WIFI_AP_LIB);   
    lua_setfield(L, -2, "ap");     

    return 1; // return table wifi
}
