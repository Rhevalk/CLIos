// ============================= IMPLEMENTATION =============================
// gpio.mode(pin, mode, pull_up?, pull_down?) 
// 
// gpio.mode(2, gpio.OUTPUT)
// gpio.write(2, 1)
// print('GPIO2:', gpio.read(2))
// ============================= IMPLEMENTATION =============================

#include "hdw_gpio.h"

int L_GPIO_MODE(lua_State *L) {
  int _pin  = luaL_checkinteger(L, 1);
  int _mode = luaL_checkinteger(L, 2);
  bool _pullUp = luaL_optinteger(L, 3, false);
  bool _pullDown = luaL_optinteger(L, 4, false);

  gpio_config_t io_conf = {
      .pin_bit_mask = (1ULL << _pin),
      .mode = _mode,
      .pull_up_en = _pullUp ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE,
      .pull_down_en = _pullDown ? GPIO_PULLDOWN_ENABLE : GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_DISABLE
  };

  
  gpio_config(&io_conf);
  
  gpio_set_level(_pin, false);
  
  lua_pushboolean(L, 1); // return true kalau sukses
  return 1;
}

int L_GPIO_WRITE(lua_State *L)
{
  int _pin  = luaL_checkinteger(L, 1);
  int _value;
  if(lua_isboolean(L, 2)) 
    _value = lua_toboolean(L, 2); // true -> 1, false -> 0
  else 
    _value = luaL_checkinteger(L, 2);

  gpio_set_level(_pin, _value);

  lua_pushboolean(L, 1); // return true kalau sukses
  return 1;
}

int L_GPIO_READ(lua_State *L)
{
  int _pin  = luaL_checkinteger(L, 1);
  int value = gpio_get_level(_pin);
  lua_pushinteger(L, value);
  return 1;
}

static const luaL_Reg gpio_funcs[] = {
    {"mode", L_GPIO_MODE},
    {"write", L_GPIO_WRITE},
    {"read", L_GPIO_READ},
    {NULL, NULL}
};

int LUA_OPEN_GPIO(lua_State *L) {
    luaL_newlib(L, gpio_funcs);

    lua_pushinteger(L, GPIO_MODE_OUTPUT); lua_setfield(L, -2,"OUTPUT");
    lua_pushinteger(L, GPIO_MODE_INPUT); lua_setfield(L, -2,"INPUT");
    lua_pushinteger(L, GPIO_MODE_INPUT_OUTPUT); lua_setfield(L, -2,"INPUT_OUTPUT");

    return 1;
}
