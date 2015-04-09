// Copyright (C) 2014-2015 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#include <uv.h>
#include <lua.h>
#include <lauxlib.h>
#include <assert.h>
#include "qsf_log.h"
#include "qsf_malloc.h"
#include "net/qsf_net_server.h"
#include "net/qsf_net_def.h"


typedef struct
{
    struct qsf_net_server_s* s;
    lua_State* L;
    int read_ref;
}net_server_t;

#define SERVER_HANDLE     "server*"
#define check_server(L)   ((net_server_t*)luaL_checkudata(L, 1, SERVER_HANDLE))

static uv_loop_t* global_loop;

//////////////////////////////////////////////////////////////////////////
// net.server interface

static int create_server(lua_State* L)
{
    assert(global_loop != NULL);
    uint32_t heart_beat_sec = NET_DEFAULT_HEARTBEAT;
    uint32_t heart_beat_check_sec = NET_DEFAULT_HEARTBEAT_CHECK;
    uint32_t max_connections = NET_DEFAULT_MAX_CONN;
    uint32_t max_buffer_size = NET_MAX_PACKET_SIZE;
    if (lua_istable(L, 1))
    {
        int top = lua_gettop(L);
        lua_getfield(L, 1, "heart_beat");
        if (lua_isinteger(L, -1))
        {
            heart_beat_sec = (uint32_t)luaL_checkinteger(L, -1);
        }
        lua_getfield(L, 1, "heart_beat_check");
        if (lua_isinteger(L, -1))
        {
            heart_beat_check_sec = (uint32_t)luaL_checkinteger(L, -1);
        }
        lua_getfield(L, 1, "max_connection");
        if (lua_isinteger(L, -1))
        {
            max_connections = (uint32_t)luaL_checkinteger(L, -1);
        }
        lua_getfield(L, 1, "buffer_size");
        if (lua_isinteger(L, -1))
        {
            max_buffer_size = (uint32_t)luaL_checkinteger(L, -1);
        }
        lua_pop(L, lua_gettop(L) - top);
    }

    struct qsf_net_server_s* s = qsf_create_net_server(global_loop, max_connections,
        heart_beat_sec, heart_beat_check_sec, max_buffer_size);
    net_server_t* server = lua_newuserdata(L, sizeof(net_server_t));
    server->s = s;
    server->L = L;
    server->read_ref = LUA_NOREF;
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

static void on_read(int err, uint32_t serial, const char* data, uint16_t size, void* ud)
{
    assert(data && size);
    net_server_t* server = ud;
    int ref = server->read_ref;
    lua_State* L = server->L;
    lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
    assert(lua_isfunction(L, -1));
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
    if (lua_pcall(L, 3, 0, 0) != LUA_OK)
    {
        qsf_log(lua_tostring(L, -1));
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
    qsf_net_set_server_udata(server->s, server);
    int r = qsf_net_server_start(server->s, host, port, on_read);
    if (r < 0)
    {
        luaL_error(L, "net.server start failed: ", uv_strerror(r));
    }
    return 0;
}

static int server_stop(lua_State* L)
{
    return 0;
}

//////////////////////////////////////////////////////////////////////////
// net interface 

static int net_poll(lua_State* L)
{
    int r = uv_run(global_loop, UV_RUN_NOWAIT);
    lua_pushinteger(L, r);
    return 1;
}

static int net_stop(lua_State* L)
{
    uv_stop(global_loop);
    return 0;
}

static void make_meta(lua_State* L)
{
    static const luaL_Reg server_lib[] =
    {
        { "__gc", server_gc },
        { "start", server_start },

        { NULL, NULL },
    };
    luaL_newmetatable(L, SERVER_HANDLE);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, server_lib, 0);
    lua_pop(L, 1);  /* pop new metatable */
}

LUALIB_API int luaopen_net(lua_State* L)
{
    global_loop = uv_default_loop();
    static const luaL_Reg lib[] = 
    {
        { "create_server", create_server },
        { "poll", net_poll },
        { "stop", net_stop },
        {NULL, NULL}
    };
    luaL_newlib(L, lib);
    make_meta(L);
    return 1;
}