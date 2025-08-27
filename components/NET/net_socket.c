#include "net_socket.h"

// ================================ TCP Methods ============================
static int F_CONNECT(lua_State *L){
    lua_sock_t* s = (lua_sock_t*)luaL_checkudata(L,1,"net.sock");
    const char* host = luaL_checkstring(L,2);
    int port = luaL_checkinteger(L,3);

    struct addrinfo hints = {0}, *res;
    hints.ai_socktype = SOCK_STREAM;
    char p[8]; snprintf(p,sizeof(p),"%d",port);

    if(getaddrinfo(host,p,&hints,&res)!=0)
        return luaL_error(L,"getaddrinfo failed");

    s->fd = socket(res->ai_family,SOCK_STREAM,0);
    if(s->fd < 0){
        freeaddrinfo(res);
        return luaL_error(L,"socket failed");
    }

    if(connect(s->fd,res->ai_addr,res->ai_addrlen)!=0){
        freeaddrinfo(res);
        return luaL_error(L,"connect failed");
    }

    freeaddrinfo(res);
    lua_pushboolean(L,1);
    return 1;
}

static int F_LISTEN(lua_State *L){
    lua_sock_t* s = (lua_sock_t*)luaL_checkudata(L,1,"net.sock");
    const char* bind_ip = luaL_optstring(L,2,"0.0.0.0");
    int port = luaL_checkinteger(L,3);

    s->fd = socket(AF_INET,SOCK_STREAM,0);
    struct sockaddr_in a = {0};
    a.sin_family = AF_INET;
    a.sin_port = htons(port);
    a.sin_addr.s_addr = inet_addr(bind_ip);

    int yes = 1;
    setsockopt(s->fd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(yes));

    if(bind(s->fd,(struct sockaddr*)&a,sizeof(a))!=0 || listen(s->fd,2)!=0)
        return luaL_error(L,"bind/listen failed");

    lua_pushboolean(L,1);
    return 1;
}

static int F_ACCEPT(lua_State *L){
    lua_sock_t* s = (lua_sock_t*)luaL_checkudata(L,1,"net.sock");
    int c = accept(s->fd,NULL,NULL);
    if(c < 0) return luaL_error(L,"accept failed");

    lua_sock_t* ns = (lua_sock_t*)lua_newuserdata(L,sizeof(*ns));
    ns->fd = c; ns->is_udp = 0;
    luaL_getmetatable(L,"net.sock");
    lua_setmetatable(L,-2);
    return 1;
}

static int F_SEND(lua_State *L){
    lua_sock_t* s = (lua_sock_t*)luaL_checkudata(L,1,"net.sock");
    size_t n;
    const char* buf = luaL_checklstring(L,2,&n);
    int w = send(s->fd,buf,n,0);
    lua_pushinteger(L,w);
    return 1;
}

static int F_RECV(lua_State *L){
    lua_sock_t* s = (lua_sock_t*)luaL_checkudata(L,1,"net.sock");
    int n = luaL_checkinteger(L,2);
    char* buf = (char*)malloc(n);
    if(!buf) return luaL_error(L,"out of memory");
    int r = recv(s->fd,buf,n,0);
    if(r<0) r=0;
    lua_pushlstring(L,buf,r);
    free(buf);
    return 1;
}

static int F_CLOSE(lua_State *L){
    lua_sock_t* s = (lua_sock_t*)luaL_checkudata(L,1,"net.sock");
    if(s->fd>=0){ close(s->fd); s->fd=-1; }
    lua_pushboolean(L,1);
    return 1;
}

// ================================ UDP Methods ============================
static int F_BIND(lua_State *L){
    lua_sock_t* s = (lua_sock_t*)luaL_checkudata(L,1,"net.sock.udp");
    const char* ip = luaL_optstring(L,2,"0.0.0.0");
    int port = luaL_checkinteger(L,3);

    struct sockaddr_in a = {0};
    a.sin_family = AF_INET;
    a.sin_port = htons(port);
    a.sin_addr.s_addr = inet_addr(ip);

    if(bind(s->fd,(struct sockaddr*)&a,sizeof(a))!=0)
        return luaL_error(L,"bind failed");

    lua_pushboolean(L,1);
    return 1;
}

static int F_SENDTO(lua_State *L){
    lua_sock_t* s = (lua_sock_t*)luaL_checkudata(L,1,"net.sock.udp");
    size_t n;
    const char* buf = luaL_checklstring(L,2,&n);
    const char* ip = luaL_checkstring(L,3);
    int port = luaL_checkinteger(L,4);

    struct sockaddr_in a = {0};
    a.sin_family = AF_INET;
    a.sin_port = htons(port);
    a.sin_addr.s_addr = inet_addr(ip);

    int w = sendto(s->fd,buf,n,0,(struct sockaddr*)&a,sizeof(a));
    lua_pushinteger(L,w);
    return 1;
}

static int F_RECVFROM(lua_State *L){
    lua_sock_t* s = (lua_sock_t*)luaL_checkudata(L,1,"net.sock.udp");
    int n = luaL_checkinteger(L,2);
    char* buf = (char*)malloc(n);
    if(!buf) return luaL_error(L,"out of memory");

    struct sockaddr_in a; socklen_t alen = sizeof(a);
    int r = recvfrom(s->fd,buf,n,0,(struct sockaddr*)&a,&alen);
    if(r<0) r=0;
    lua_pushlstring(L,buf,r);
    free(buf);
    return 1;
}

static int F_CLOSE_UDP(lua_State *L){
    lua_sock_t* s = (lua_sock_t*)luaL_checkudata(L,1,"net.sock.udp");
    if(s->fd>=0){ close(s->fd); s->fd=-1; }
    lua_pushboolean(L,1);
    return 1;
}

// ================================ Lua constructors =======================
static int L_SOCK_TCP(lua_State *L){
    lua_sock_t* s = (lua_sock_t*)lua_newuserdata(L,sizeof(*s));
    s->fd=-1; s->is_udp=0;

    if(luaL_newmetatable(L,"net.sock")){
        luaL_Reg mt[] = {
            {"connect",F_CONNECT},
            {"listen",F_LISTEN},
            {"accept",F_ACCEPT},
            {"send",F_SEND},
            {"recv",F_RECV},
            {"close",F_CLOSE},
            {NULL,NULL}
        };
        luaL_setfuncs(L,mt,0);
        lua_pushvalue(L,-1);
        lua_setfield(L,-2,"__index");
    }

    lua_setmetatable(L,-2);
    return 1;
}

static int L_SOCK_UDP(lua_State *L){
    lua_sock_t* s = (lua_sock_t*)lua_newuserdata(L,sizeof(*s));
    s->fd = socket(AF_INET,SOCK_DGRAM,0);
    s->is_udp=1;

    if(luaL_newmetatable(L,"net.sock.udp")){
        luaL_Reg mt[] = {
            {"bind",F_BIND},
            {"sendto",F_SENDTO},
            {"recvfrom",F_RECVFROM},
            {"close",F_CLOSE_UDP},
            {NULL,NULL}
        };
        luaL_setfuncs(L,mt,0);
        lua_pushvalue(L,-1);
        lua_setfield(L,-2,"__index");
    }

    lua_setmetatable(L,-2);
    return 1;
}

// ================================ Library Registration ===================
static const luaL_Reg SOCK_SUBLIB[] = {
    {"tcp",L_SOCK_TCP},
    {"udp",L_SOCK_UDP},
    {NULL,NULL}
};

int LUA_OPEN_SOCKET(lua_State *L){
    luaL_newlib(L,SOCK_SUBLIB);
    return 1;
}
