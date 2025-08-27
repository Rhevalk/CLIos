#include "net_mdns.h"

// ================================ mDNS ===================================
// mdns.start(hostname[, instance]) ; mdns.add_service(name, type, proto, port)

static int L_MDNS_START(lua_State *L){ const char* host=luaL_checkstring(L,1); const char* inst=luaL_optstring(L,2, host); ESP_ERROR_CHECK(mdns_init()); ESP_ERROR_CHECK(mdns_hostname_set(host)); ESP_ERROR_CHECK(mdns_instance_name_set(inst)); lua_pushboolean(L,1); return 1; }
static int L_MDNS_ADD(lua_State *L){ const char* name=luaL_checkstring(L,1); const char* type=luaL_checkstring(L,2); const char* proto=luaL_optstring(L,3,"_tcp"); int port=luaL_checkinteger(L,4); ESP_ERROR_CHECK(mdns_service_add(name, type, proto, port, NULL, 0)); lua_pushboolean(L,1); return 1; }
static const luaL_Reg MDNS_LIB[]={{"start",L_MDNS_START},{"add_service",L_MDNS_ADD},{NULL,NULL}};
int LUA_OPEN_MDNS(lua_State *L) { luaL_newlib(L, MDNS_LIB); return 1; }