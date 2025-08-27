#pragma once


#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

#include "lua.h"
#include "lauxlib.h"



typedef struct { int fd; int is_udp; } lua_sock_t;

int LUA_OPEN_SOCKET(lua_State *L);