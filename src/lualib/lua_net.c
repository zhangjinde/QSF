// Copyright (C) 2014-2015 chenqiang@chaoyuehudong.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#include <uv.h>
#include <lua.h>
#include <lauxlib.h>
#include <assert.h>
#include "qsf.h"
#include "net/qsf_net_def.h"
#include "net/qsf_net_server.h"


#define SERVER_HANDLE     "server*"
#define check_server(L)   ((net_server_t*)luaL_checkudata(L, 1, SERVER_HANDLE))


typedef struct
{
    qsf_net_server_t* s;
    lua_State* L;
    int read_ref;
}net_server_t;

static uv_loop_t* get_loop(lua_State* L)
{
    qsf_node_t* self = lua_touserdata(L, lua_upvalueindex(1));
    if (!qsf_node_check_tag(self))
    {
        luaL_error(L, "invalid node object");
    }
    return qsf_node_loop(self);
}

//////////////////////////////////////////////////////////////////////////
// net.server interface

static int create_server(lua_State* L)
{
    uv_loop_t* loop = get_loop(L);
    uint32_t max_connections = (uint32_t)luaL_optinteger(L, 1, NET_DEFAULT_MAX_CONN);
    uint16_t heartbeat_sec = (uint16_t)luaL_optinteger(L, 2, NET_DEFAULT_HEARTBEAT);
    uint16_t heartbeat_check_sec = (uint16_t)luaL_optinteger(L, 3, NET_DEFAULT_HEARTBEAT_CHECK);
    struct qsf_net_server_s* s = qsf_create_net_server(loop, max_connections,
        heartbeat_sec, heartbeat_check_sec);
    net_server_t* server = lua_newuserdata(L, sizeof(net_server_t));
    server->s = s;
    server->L = L;
    server->read_ref = LUA_NOREF;
    qsf_net_set_server_udata(server->s, server);
    luaL_getmetatable(L, SERVER_HANDLE);
    lua_setmetatable(L, -2);
    return 1;
}

static int server_gc(lua_State* L)
{
    net_server_t* server = check_server(L);
    qsf_net_server_destroy(server->s);
    luaL_unref(L, LUA_REGISTRYINDEX, server->read_ref);
    return 0;
}

static void on_server_read(int err, uint32_t serial, const char* data, uint16_t size, void* ud)
{
    assert(data && size);
    net_server_t* server = ud;
    int ref = server->read_ref;
    lua_State* L = server->L;
    lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
    if (lua_isfunction(L, -1))
    {
        if (err == 0)
        {
            lua_pushnil(L);
        }
        else
        {
            lua_pushinteger(L, err);
        }
        lua_pushinteger(L, serial);
        lua_pushlstring(L, data, size);
        qsf_trace_pcall(L, 3);
    }
}

static int server_start(lua_State* L)
{
    net_server_t* server = check_server(L);
    luaL_unref(L, LUA_REGISTRYINDEX, server->read_ref);

    const char* host = luaL_checkstring(L, 2);
    int port = (int)luaL_checkinteger(L, 3);

    luaL_argcheck(L, lua_isfunction(L, 4), 4, "read callback must be function type");
    int read_ref = luaL_ref(L, LUA_REGISTRYINDEX);
    qsf_assert(read_ref != LUA_NOREF, "luaL_ref() failed.");

    server->read_ref = read_ref;
    server->L = L;
    int r = qsf_net_server_start(server->s, host, port, on_server_read);
    if (r < 0)
    {
        luaL_error(L, "net.server start failed: %s", uv_strerror(r));
    }
    return 0;
}

static int server_stop(lua_State* L)
{
    net_server_t* server = check_server(L);
    qsf_net_server_stop(server->s);
    return 0;
}

static int server_write(lua_State* L)
{
    net_server_t* server = check_server(L);
    uint32_t serial = (uint32_t)luaL_checkinteger(L, 2);
    size_t size;
    const char* data = luaL_checklstring(L, 3, &size);
    if (size <= UINT16_MAX)
    {
        qsf_net_server_write(server->s, serial, data, (uint16_t)size);
        return 0;
    }
    return luaL_error(L, "too big packet to write: %d/%d", size, UINT16_MAX);
}

static int server_broadcast(lua_State* L)
{
    net_server_t* server = check_server(L);
    size_t size;
    const char* data = luaL_checklstring(L, 2, &size);
    if (size <= UINT16_MAX)
    {
        qsf_net_server_write_all(server->s, data, (uint16_t)size);
        return 0;
    }
    return luaL_error(L, "too big packet to write: %d/%d", size, UINT16_MAX);
}

static int server_shutdown(lua_State* L)
{
    net_server_t* server = check_server(L);
    uint32_t serial = (uint32_t)luaL_checkinteger(L, 2);
    qsf_net_server_shutdown(server->s, serial);
    return 0;
}

static int server_close(lua_State* L)
{
    net_server_t* server = check_server(L);
    uint32_t serial = (uint32_t)luaL_checkinteger(L, 2);
    qsf_net_server_close(server->s, serial);
    return 0;
}

static int server_address_of(lua_State* L)
{
    net_server_t* server = check_server(L);
    uint32_t serial = (uint32_t)luaL_checkinteger(L, 2);
    char address[20] = { '\0' };
    qsf_net_server_session_address(server->s, serial, address, sizeof(address));
    lua_pushstring(L, address);
    return 1;
}

static int server_size(lua_State* L)
{
    net_server_t* server = check_server(L);
    int size = qsf_net_server_size(server->s);
    lua_pushinteger(L, size);
    return 1;
}

//////////////////////////////////////////////////////////////////////////
// net interface 

static void make_meta(lua_State* L)
{
    static const luaL_Reg lib[] =
    {
        { "__gc", server_gc },
        { "start", server_start },
        { "stop", server_stop },
        { "write", server_write },
        { "broadcast", server_broadcast },
        { "shutdown", server_shutdown},
        { "kick", server_close },
        { "addressOf", server_address_of},
        { "size", server_size },
        { NULL, NULL },
    };
    luaL_newmetatable(L, SERVER_HANDLE);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, lib, 0);
    lua_pop(L, 1);  /* pop new metatable */    
}

LUALIB_API int luaopen_net(lua_State* L)
{
    static const luaL_Reg lib[] = 
    {
        { "createServer", create_server },
        {NULL, NULL}
    };
    luaL_newlibtable(L, lib);
    lua_getfield(L, LUA_REGISTRYINDEX, "qsf_ctx");
    qsf_node_t* self = lua_touserdata(L, -1);
    luaL_argcheck(L, self != NULL, 1, "invalid context pointer");
    luaL_setfuncs(L, lib, 1);
    make_meta(L);
    return 1;
}
