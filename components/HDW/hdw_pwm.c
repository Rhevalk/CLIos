// ============================= IMPLEMENTATION =============================
// local p = pwm.new(pin, freq?, duty?, resolution?, channel?, timer?, speedmode?)
//
// local led = pwm.new(2, 1000, 0, 10, pwm.CH0, pwm.TIMER_0, pwm.LOW_SPEED)
// 
// led:start()
// 
// for duty = 0, 1023, 64 do
//     led:set_duty(duty)
//     print("Duty:", duty)
//     task.delay(200) -- delay 200ms
// end
// 
// led:stop()
// led:free()
// ============================= IMPLEMENTATION =============================

#include "hdw_pwm.h"

// metode untuk objek PWM
static int l_pwm_set_duty(lua_State *L) {
    lua_pwm_t *p = (lua_pwm_t*)luaL_checkudata(L,1,"hw.pwm");
    int duty = luaL_checkinteger(L, 2);
    ESP_ERROR_CHECK(ledc_set_duty(p->ccfg.speed_mode, p->ccfg.channel, duty));
    ESP_ERROR_CHECK(ledc_update_duty(p->ccfg.speed_mode, p->ccfg.channel));
    lua_pushboolean(L,1);
    return 1;
}

static int l_pwm_set_freq(lua_State *L) {
    lua_pwm_t *p = (lua_pwm_t*)luaL_checkudata(L,1,"hw.pwm");
    int freq = luaL_checkinteger(L, 2);
    ESP_ERROR_CHECK(ledc_set_freq(p->ccfg.speed_mode, p->ccfg.timer_sel, freq));
    lua_pushboolean(L,1);
    return 1;
}

static int l_pwm_stop(lua_State *L) {
    lua_pwm_t *p = (lua_pwm_t*)luaL_checkudata(L,1,"hw.pwm");
    ESP_ERROR_CHECK(ledc_stop(p->ccfg.speed_mode, p->ccfg.channel, 0));
    p->started = false;
    lua_pushboolean(L,1);
    return 1;
}

static int l_pwm_start(lua_State *L) {
    lua_pwm_t *p = (lua_pwm_t*)luaL_checkudata(L,1,"hw.pwm");
    ESP_ERROR_CHECK(ledc_update_duty(p->ccfg.speed_mode, p->ccfg.channel));
    p->started = true;
    lua_pushboolean(L,1);
    return 1;
}

static int l_pwm_free(lua_State *L) {
    lua_pwm_t *p = (lua_pwm_t*)luaL_checkudata(L,1,"hw.pwm");
    ledc_stop(p->ccfg.speed_mode, p->ccfg.channel, 0);
    lua_pushboolean(L,1);
    return 1;
}

// konstruktor simple: pwm.new(pin, freq, duty, resolution, channel, timer, speedmode)
static int l_pwm_new(lua_State *L) {
    int pin       = luaL_checkinteger(L, 1);
    int freq      = luaL_optinteger(L, 2, 1000);
    int duty      = luaL_optinteger(L, 3, 0);
    int res_bits  = luaL_optinteger(L, 4, 10);
    int channel   = luaL_optinteger(L, 5, LEDC_CHANNEL_0);
    int timer     = luaL_optinteger(L, 6, LEDC_TIMER_0);
    int speedmode = luaL_optinteger(L, 7, LEDC_LOW_SPEED_MODE);

    lua_pwm_t *p = (lua_pwm_t*)lua_newuserdata(L, sizeof(*p));
    memset(p, 0, sizeof(*p));

    p->tcfg = (ledc_timer_config_t){
        .speed_mode      = speedmode,
        .duty_resolution = res_bits,
        .timer_num       = timer,
        .freq_hz         = (uint32_t)freq,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&p->tcfg));

    p->ccfg = (ledc_channel_config_t){
        .gpio_num        = pin,
        .speed_mode      = speedmode,
        .channel         = channel,
        .timer_sel       = timer,
        .duty            = duty,
        .hpoint          = 0,
        .flags.output_invert = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&p->ccfg));

    if (luaL_newmetatable(L, "hw.pwm")) {
        lua_pushcfunction(L, l_pwm_set_duty); lua_setfield(L, -2, "set_duty");
        lua_pushcfunction(L, l_pwm_set_freq); lua_setfield(L, -2, "set_freq");
        lua_pushcfunction(L, l_pwm_stop);     lua_setfield(L, -2, "stop");
        lua_pushcfunction(L, l_pwm_start);    lua_setfield(L, -2, "start");
        lua_pushcfunction(L, l_pwm_free);     lua_setfield(L, -2, "free");
        lua_pushvalue(L, -1); lua_setfield(L, -2, "__index");
    }
    lua_setmetatable(L, -2);
    return 1;
}

// registrasi modul
int LUA_OPEN_PWM(lua_State *L) {
    lua_newtable(L);
    lua_pushcfunction(L, l_pwm_new); lua_setfield(L, -2, "new");

    // expose enums
    lua_pushinteger(L, LEDC_LOW_SPEED_MODE); lua_setfield(L, -2, "LOW_SPEED");
    lua_pushinteger(L, LEDC_HIGH_SPEED_MODE);lua_setfield(L, -2, "HIGH_SPEED");
    lua_pushinteger(L, LEDC_TIMER_0); lua_setfield(L, -2, "TIMER_0");
    lua_pushinteger(L, LEDC_TIMER_1); lua_setfield(L, -2, "TIMER_1");
    lua_pushinteger(L, LEDC_TIMER_2); lua_setfield(L, -2, "TIMER_2");
    lua_pushinteger(L, LEDC_TIMER_3); lua_setfield(L, -2, "TIMER_3");
    lua_pushinteger(L, LEDC_CHANNEL_0); lua_setfield(L, -2, "CH0");
    lua_pushinteger(L, LEDC_CHANNEL_1); lua_setfield(L, -2, "CH1");
    lua_pushinteger(L, LEDC_CHANNEL_2); lua_setfield(L, -2, "CH2");
    lua_pushinteger(L, LEDC_CHANNEL_3); lua_setfield(L, -2, "CH3");
    lua_pushinteger(L, LEDC_CHANNEL_4); lua_setfield(L, -2, "CH4");
    lua_pushinteger(L, LEDC_CHANNEL_5); lua_setfield(L, -2, "CH5");
    lua_pushinteger(L, LEDC_CHANNEL_6); lua_setfield(L, -2, "CH6");
    lua_pushinteger(L, LEDC_CHANNEL_7); lua_setfield(L, -2, "CH7");

    return 1;
}
