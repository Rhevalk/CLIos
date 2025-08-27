// ble_ble.c
#include "ble_ble.h"

/* Small nimble init state */
static bool g_nimble_ready = false;
static int g_conn_handle = -1;

/* forward declarations */
static void ble_on_sync(void);
static int ble_gap_event(struct ble_gap_event *event, void *arg);
static int lua_ble_init(lua_State *L);
static int lua_ble_start_advertise(lua_State *L);
static int lua_ble_stop_advertise(lua_State *L);

/* Simple wrapper to push boolean return values to Lua:  true on success, false+errmsg on fail */
static int lua_push_ok_or_err(lua_State *L, esp_err_t res, const char *errmsg) {
    if (res == ESP_OK) {
        lua_pushboolean(L, 1);
        return 1;
    } else {
        lua_pushboolean(L, 0);
        lua_pushstring(L, errmsg ? errmsg : esp_err_to_name(res));
        return 2;
    }
}


static const uint8_t hid_report_map[] = {
    /* ============== Keyboard (Report ID 1) ============== */
    0x05, 0x01,       /* Usage Page (Generic Desktop) */
    0x09, 0x06,       /* Usage (Keyboard) */
    0xA1, 0x01,       /* Collection (Application) */
    0x85, 0x01,       /*   Report ID (1) */
    0x05, 0x07,       /*   Usage Page (Key Codes) */
    0x19, 0xE0,       /*   Usage Minimum (224) */
    0x29, 0xE7,       /*   Usage Maximum (231) */
    0x15, 0x00,       /*   Logical Minimum (0) */
    0x25, 0x01,       /*   Logical Maximum (1) */
    0x75, 0x01,       /*   Report Size (1) */
    0x95, 0x08,       /*   Report Count (8) -> modifier byte */
    0x81, 0x02,       /*   Input (Data,Var,Abs) */
    0x95, 0x01,       /*   Report Count (1) -> reserved */
    0x75, 0x08,       /*   Report Size (8) */
    0x81, 0x01,       /*   Input (Const,Array,Abs) */
    0x95, 0x06,       /*   Report Count (6) -> up to 6 keys */
    0x75, 0x08,       /*   Report Size (8) */
    0x15, 0x00,       /*   Logical Minimum (0) */
    0x25, 0x65,       /*   Logical Maximum (101) */
    0x05, 0x07,       /*   Usage Page (Key Codes) */
    0x19, 0x00,       /*   Usage Minimum (0) */
    0x29, 0x65,       /*   Usage Maximum (101) */
    0x81, 0x00,       /*   Input (Data,Array) */
    0xC0,             /* End Collection */

    /* ============== Mouse (Report ID 2) ============== */
    0x05, 0x01,       /* Usage Page (Generic Desktop) */
    0x09, 0x02,       /* Usage (Mouse) */
    0xA1, 0x01,       /* Collection (Application) */
    0x85, 0x02,       /*   Report ID (2) */
    0x09, 0x01,       /*   Usage (Pointer) */
    0xA1, 0x00,       /*   Collection (Physical) */
    0x05, 0x09,       /*     Usage Page (Buttons) */
    0x19, 0x01,       /*     Usage Minimum (1) */
    0x29, 0x03,       /*     Usage Maximum (3) */
    0x15, 0x00,       /*     Logical Minimum (0) */
    0x25, 0x01,       /*     Logical Maximum (1) */
    0x95, 0x03,       /*     Report Count (3) */
    0x75, 0x01,       /*     Report Size (1) */
    0x81, 0x02,       /*     Input (Data,Var,Abs) */
    0x95, 0x01,       /*     Report Count (1) */
    0x75, 0x05,       /*     Report Size (5) */
    0x81, 0x01,       /*     Input (Const,Array,Abs) */
    0x05, 0x01,       /*     Usage Page (Generic Desktop) */
    0x09, 0x30,       /*     Usage (X) */
    0x09, 0x31,       /*     Usage (Y) */
    0x09, 0x38,       /*     Usage (Wheel) */
    0x15, 0x81,       /*     Logical Minimum (-127) */
    0x25, 0x7F,       /*     Logical Maximum (127) */
    0x75, 0x08,       /*     Report Size (8) */
    0x95, 0x03,       /*     Report Count (3) */
    0x81, 0x06,       /*     Input (Data,Var,Rel) */
    0xC0,             /*   End Collection */
    0xC0              /* End Collection */
};


static uint16_t hid_report_char_handle_kb __attribute__((unused)) = 0;
static uint16_t hid_report_char_handle_mouse __attribute__((unused)) = 0;

/* GATT access callback */
static int gatt_svr_chr_access_cb(uint16_t conn_handle, uint16_t attr_handle,
                                  struct ble_gatt_access_ctxt *ctxt, void *arg) {
    (void) arg;
    (void) conn_handle;

    if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
        int rc;
        struct os_mbuf *om = ctxt->om;
        int len = OS_MBUF_PKTLEN(om);
        uint8_t buf[256];
        if (len > (int)sizeof(buf)) len = sizeof(buf);
        rc = ble_hs_mbuf_to_flat(om, buf, len, NULL);
        if (rc == 0) {
            LOG_I("lua_nimble", "GATT RX write, len=%d", len);
            /* here you can push to a queue or handle data; calling Lua directly from
             * NimBLE context is unsafe unless synchronized — keep it outside for now.
             */
        }
        return 0;
    }

    if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
        /* not permitted in this simple example */
        return BLE_ATT_ERR_READ_NOT_PERMITTED;
    }
    return 0;
}


/* Tambah HID Report Map ke GATT */
static int hid_report_map_access_cb(uint16_t conn_handle, uint16_t attr_handle,
                                    struct ble_gatt_access_ctxt *ctxt, void *arg) {
    int rc = os_mbuf_append(ctxt->om, hid_report_map, sizeof(hid_report_map));
    return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
}

/* HID Service */
static struct ble_gatt_svc_def hid_svc[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = BLE_UUID16_DECLARE(0x1812), /* HID Service */
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                .uuid = BLE_UUID16_DECLARE(0x2A4B), /* Report Map */
                .access_cb = hid_report_map_access_cb,
                .flags = BLE_GATT_CHR_F_READ,
            },
            {
                .uuid = BLE_UUID16_DECLARE(0x2A4D), /* Report (Keyboard) */
                .access_cb = gatt_svr_chr_access_cb,
                .flags = BLE_GATT_CHR_F_NOTIFY,
                .val_handle = &hid_report_char_handle_kb,
            },
            {
                .uuid = BLE_UUID16_DECLARE(0x2A4D), /* Report (Mouse) */
                .access_cb = gatt_svr_chr_access_cb,
                .flags = BLE_GATT_CHR_F_NOTIFY,
                .val_handle = &hid_report_char_handle_mouse,
            },
            {0}
        }
    },
    {0}
};

/* ----------------------------- NimBLE Init ----------------------------- */

static void ble_on_sync(void) {
    LOG_I("lua_nimble", "NimBLE synchronized");
    g_nimble_ready = true;
}

static void nimble_host_task(void *param) {
    (void)param;
    /* nimble_port_freertos_init will create host task; keep placeholder */
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

static int lua_ble_init(lua_State *L) {
    if (g_nimble_ready) {
        lua_pushboolean(L, 1);
        return 1;
    }

    /* Disable classic BT to save RAM */
    esp_err_t ret = esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);
    if (ret != ESP_OK) {
        return lua_push_ok_or_err(L, ret, "bt_controller_mem_release failed");
    }
    
    /* Init NimBLE host stack (ESP-IDF v5) */
    ret = nimble_port_init();
    if (ret != 0) {
        return lua_push_ok_or_err(L, ESP_FAIL, "nimble_port_init failed");
    }
    


    /* Init nimble stack */
    ret = nimble_port_init();
    if (ret != 0) {
        return lua_push_ok_or_err(L, ESP_FAIL, "nimble_port_init failed");
    }

    /* Configure callbacks */
    ble_hs_cfg.reset_cb = NULL;
    ble_hs_cfg.sync_cb = ble_on_sync;
    ble_hs_cfg.gatts_register_cb = NULL;

    /* Start freertos host */
    nimble_port_freertos_init(nimble_host_task);

    /* wait briefly for sync */
    int wait = 0;
    while (!g_nimble_ready && wait++ < 50) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    if (!g_nimble_ready) {
        return lua_push_ok_or_err(L, ESP_FAIL, "nimble sync timeout");
    }

    lua_pushboolean(L, 1);
    return 1;
}

/* ----------------------------- GAP (advertise/connect) ----------------------------- */

static int ble_gap_event(struct ble_gap_event *event, void *arg) {
    (void)arg;
    switch (event->type) {
    case BLE_GAP_EVENT_CONNECT:
        if (event->connect.status == 0) {
            g_conn_handle = event->connect.conn_handle;
            LOG_I("lua_nimble", "connected, handle=%d", g_conn_handle);
        } else {
            LOG_W("lua_nimble", "connect failed; status=%d", event->connect.status);
        }
        break;
    case BLE_GAP_EVENT_DISCONNECT:
        LOG_I("lua_nimble", "disconnected (reason=%d)", event->disconnect.reason);
        g_conn_handle = -1;
        /* optionally restart advertising here */
        break;
    default:
        break;
    }
    return 0;
}

static int lua_ble_start_advertise(lua_State *L) {
    const char *name = NULL;
    uint32_t interval_ms = 100; /* default */

    if (lua_gettop(L) >= 1 && lua_type(L,1) == LUA_TSTRING) {
        name = lua_tostring(L,1);
    }
    if (lua_gettop(L) >= 2 && lua_isinteger(L,2)) {
        interval_ms = (uint32_t)lua_tointeger(L,2);
    }

    struct ble_gap_adv_params adv_params;
    memset(&adv_params, 0, sizeof(adv_params));
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

    struct ble_hs_adv_fields fields;
    memset(&fields, 0, sizeof(fields));
    /* Flags */
    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;

    if (name) {
        fields.name = (const uint8_t *)name;
        fields.name_len = (uint8_t)strlen(name);
        fields.name_is_complete = 1;
    }

    int rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0) {
        return luaL_error(L, "ble_gap_adv_set_fields failed rc=%d", rc);
    }

    rc = ble_gap_adv_start(BLE_OWN_ADDR_PUBLIC, NULL, interval_ms, &adv_params, ble_gap_event, NULL);
    if (rc != 0) {
        return luaL_error(L, "ble_gap_adv_start failed rc=%d", rc);
    }

    lua_pushboolean(L, 1);
    return 1;
}

static int lua_ble_stop_advertise(lua_State *L) {
    int rc = ble_gap_adv_stop();
    if (rc != 0) {
        return luaL_error(L, "ble_gap_adv_stop failed rc=%d", rc);
    }
    lua_pushboolean(L, 1);
    return 1;
}

/* ----------------------------- GATT + UART Service ----------------------------- */
/* UUIDs used (Nordic UART) */
static const ble_uuid128_t nus_svc_uuid = BLE_UUID128_INIT(
    0x9E,0xCA,0xDC,0x24,0x0E,0xE5,0xA9,0xE0,0x93,0xF3,0xA3,0xB5,0x01,0x00,0x40,0x6E);
static const ble_uuid128_t nus_tx_uuid = BLE_UUID128_INIT(
    0x9E,0xCA,0xDC,0x24,0x0E,0xE5,0xA9,0xE0,0x93,0xF3,0xA3,0xB5,0x02,0x00,0x40,0x6E);
static const ble_uuid128_t nus_rx_uuid = BLE_UUID128_INIT(
    0x9E,0xCA,0xDC,0x24,0x0E,0xE5,0xA9,0xE0,0x93,0xF3,0xA3,0xB5,0x03,0x00,0x40,0x6E);

/* Keep track of attribute handles */
static uint16_t g_nus_tx_handle = 0; /* notify characteristic value handle */
static uint16_t g_nus_rx_handle __attribute__((unused)) = 0; /* write characteristic value handle */

/* GATT service definition */
static struct ble_gatt_svc_def gatt_svr_svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = (ble_uuid_t*)&nus_svc_uuid,
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                .uuid = (ble_uuid_t*)&nus_tx_uuid,
                .access_cb = gatt_svr_chr_access_cb,
                .flags = BLE_GATT_CHR_F_NOTIFY,
            },
            {
                .uuid = (ble_uuid_t*)&nus_rx_uuid,
                .access_cb = gatt_svr_chr_access_cb,
                .flags = BLE_GATT_CHR_F_WRITE_NO_RSP | BLE_GATT_CHR_F_WRITE,
            },
            { 0 }
        }
    },
    { 0 }
};


static int lua_ble_gatt_register(lua_State *L) {
    int rc;

    /* Hitung dan daftarkan service UART */
    rc = ble_gatts_count_cfg(gatt_svr_svcs);
    if (rc != 0) {
        return lua_push_ok_or_err(L, ESP_FAIL, "count_cfg uart failed");
    }
    rc = ble_gatts_add_svcs(gatt_svr_svcs);
    if (rc != 0) {
        return lua_push_ok_or_err(L, ESP_FAIL, "add_svcs uart failed");
    }

    /* Hitung dan daftarkan service HID */
    rc = ble_gatts_count_cfg(hid_svc);
    if (rc != 0) {
        return lua_push_ok_or_err(L, ESP_FAIL, "count_cfg hid failed");
    }
    rc = ble_gatts_add_svcs(hid_svc);
    if (rc != 0) {
        return lua_push_ok_or_err(L, ESP_FAIL, "add_svcs hid failed");
    }

    /* Start all services */
    rc = ble_gatts_start();
    if (rc != 0) {
        return lua_push_ok_or_err(L, ESP_FAIL, "gatts_start failed");
    }

    lua_pushboolean(L, 1);
    return 1;
}

static int lua_ble_uart_send(lua_State *L) {
    size_t len;
    const char *data = luaL_checklstring(L, 1, &len);
    if (g_conn_handle < 0) {
        lua_pushboolean(L, 0);
        lua_pushstring(L, "no connection");
        return 2;
    }

    struct os_mbuf *om = ble_hs_mbuf_from_flat((const void *)data, len);
    if (!om) {
        return lua_push_ok_or_err(L, ESP_FAIL, "mbuf alloc failed");
    }
    int rc = ble_gattc_notify_custom(g_conn_handle, g_nus_tx_handle, om);
    if (rc != 0) {
        return lua_push_ok_or_err(L, ESP_FAIL, "notify failed");
    }
    lua_pushboolean(L, 1);
    return 1;
}


/* ----------------------------- HID over GATT (Keyboard + Mouse) ----------------------------- */

static esp_err_t hid_send_report(uint16_t report_handle, const uint8_t *buf, size_t len) {
    if (g_conn_handle < 0) return ESP_ERR_INVALID_STATE;
    struct os_mbuf *om = ble_hs_mbuf_from_flat((const void *)buf, len);
    if (!om) return ESP_ERR_NO_MEM;
    int rc = ble_gattc_notify_custom(g_conn_handle, report_handle, om);
    return rc == 0 ? ESP_OK : ESP_FAIL;
}

static int lua_ble_hid_send_key(lua_State *L) {
    if (!lua_isinteger(L,1)) return luaL_error(L, "modifiers must be integer");
    int modifiers = lua_tointeger(L,1);
    if (!lua_istable(L,2)) return luaL_error(L, "keycodes must be table");

    uint8_t report[9]; // 1 + 8
    memset(report,0,sizeof(report));
    report[0] = 0x01;        // Report ID = 1
    report[1] = modifiers;   // modifier
    // report[2] = reserved
    for (int i=0;i<6;i++) {
        lua_rawgeti(L,2,i+1);
        if(lua_isinteger(L,-1)) report[3+i]=(uint8_t)lua_tointeger(L,-1);
        lua_pop(L,1);
    }

    esp_err_t r = hid_send_report(hid_report_char_handle_kb, report, sizeof(report));
    if (r == ESP_OK) {
        lua_pushboolean(L, 1);
        return 1;
    }
    lua_pushboolean(L, 0);
    lua_pushstring(L, "hid send failed");
    return 2;
}

static int lua_ble_hid_send_mouse(lua_State *L) {
    int buttons = luaL_checkinteger(L,1);
    int x = luaL_checkinteger(L,2);
    int y = luaL_checkinteger(L,3);
    int wheel = 0;
    if (lua_gettop(L) >=4) wheel = luaL_checkinteger(L,4);

    uint8_t report[5];
    report[0] = 0x02;        // Report ID = 2
    report[1] = buttons & 0xFF;
    report[2] = (int8_t)x;
    report[3] = (int8_t)y;
    report[4] = (int8_t)wheel;

    esp_err_t r = hid_send_report(hid_report_char_handle_mouse, report, sizeof(report));
    if (r == ESP_OK) {
        lua_pushboolean(L, 1);
        return 1;
    }
    lua_pushboolean(L, 0);
    lua_pushstring(L, "hid send failed");
    return 2;
}

/* ----------------------------- Lua registration ----------------------------- */

static const luaL_Reg lua_ble_funcs[] = {
    {"init", lua_ble_init},
    {"start", lua_ble_start_advertise},
    {"stop", lua_ble_stop_advertise},
    {"gatt_register", lua_ble_gatt_register},
    {"uart_send", lua_ble_uart_send},
    {"hid_key", lua_ble_hid_send_key},
    {"hid_mouse", lua_ble_hid_send_mouse},
    {NULL, NULL}
};

int LUA_OPEN_BLE(lua_State *L) {
    luaL_newlib(L, lua_ble_funcs);
    return 1;
}
