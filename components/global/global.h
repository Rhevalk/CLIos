#pragma once

#include "esp32/rom/ets_sys.h" 

#define CHAR_BUFFER_SIZE 64
#define LUA_BUFFER_SIZE 256


// Gunakan ini untuk kontrol level log
#define CLI_LOG_LEVEL_NONE  0
#define CLI_LOG_LEVEL_ERROR 1
#define CLI_LOG_LEVEL_WARN  2
#define CLI_LOG_LEVEL_INFO  3
#define CLI_LOG_LEVEL_DEBUG 4

#ifndef CLI_LOG_LEVEL
#define CLI_LOG_LEVEL CLI_LOG_LEVEL_INFO   // default
#endif

// Makro dasar
#define CLI_PRINTF(fmt, ...)   ets_printf(fmt, ##__VA_ARGS__)

// Makro per level
#if CLI_LOG_LEVEL >= CLI_LOG_LEVEL_ERROR
  #define LOG_E(fmt, ...)    CLI_PRINTF("[ERROR] " fmt "\n", ##__VA_ARGS__)
#else
  #define LOG_E(fmt, ...)
#endif

#if CLI_LOG_LEVEL >= CLI_LOG_LEVEL_WARN
  #define LOG_W(fmt, ...)     CLI_PRINTF("[WARNING] " fmt "\n", ##__VA_ARGS__)
#else
  #define LOG_W(fmt, ...)
#endif

#if CLI_LOG_LEVEL >= CLI_LOG_LEVEL_INFO
  #define LOG_I(fmt, ...)     CLI_PRINTF("[SYSTEM] " fmt "\n", ##__VA_ARGS__)
#else
  #define LOG_I(fmt, ...)
#endif

#if CLI_LOG_LEVEL >= CLI_LOG_LEVEL_DEBUG
  #define LOG_D(fmt, ...)    CLI_PRINTF("[DEBUG] " fmt "\n", ##__VA_ARGS__)
#else
  #define LOG_D(fmt, ...)
#endif
