#include "gpio.h"

void SETUP_GPIO(lua_State *L)
{
  lua_pushinteger(L, GPIO_MODE_OUTPUT);
  lua_setglobal(L, "OUTPUT");
  
  lua_pushinteger(L, GPIO_MODE_INPUT);
  lua_setglobal(L, "INPUT");
  
  lua_pushinteger(L, GPIO_MODE_INPUT_OUTPUT);
  lua_setglobal(L, "INPUT_OUTPUT");
}

int L_GPIO_MODE(lua_State *L) {
  uint8_t _pin  = luaL_checkinteger(L, 1);
  uint8_t _mode = luaL_checkinteger(L, 2);
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
  uint8_t _pin  = luaL_checkinteger(L, 1);
  uint8_t _value;
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
  uint8_t _pin  = luaL_checkinteger(L, 1);
  int value = gpio_get_level(_pin);
  lua_pushinteger(L, value);
  return 1;
}

const luaL_Reg GPIO_LIB[] = {
  {"mode", L_GPIO_MODE},
  {"write", L_GPIO_WRITE},
  {"read", L_GPIO_READ},
  {NULL, NULL}
};

int LUA_OPEN_GPIO( lua_State *L ) { SETUP_GPIO(L); luaL_newlib(L, GPIO_LIB); return 1; }