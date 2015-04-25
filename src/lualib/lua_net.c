// Copyright (C) 2014-2015 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#include <uv.h>
#include <lua.h>
#include <lauxlib.h>
#include <assert.h>
#include "qsf_log.h"
#include "qsf_malloc.h"
#include "net/qsf_net_def.h"
#include "net/qsf_net_server.h"
#include "net/qsf_net_client.h"

typedef struct
{
    qsf_net_server_t* s;
    lua_State* L;
    int read_ref;
}net_server_t;

typedef struct
{
    qsf_net_client_t* c;
    lua_State* L;
    int connect_ref;
    int read_ref;
}net_client_t;

#define SERVER_HANDLE     "server*"
#define CLIENT_HANDLE     "client*"
#define check_server(L)   ((net_server_t*)luaL_checkudata(L, 1, SERVER_HANDLE))
#define check_client(L)   ((net_client_t*)luaL_checkudata(L, 1, CLIENT_HANDLE))

static uv_loop_t* global_loop;

//////////////////////////////////////////////////////////////////////////
// net.server interface

static int create_server(lua_State* L)
{
    assert(global_loop != NULL);
    uint16_t heart_beat_sec = NET_DEFAULT_HEARTBEAT;
    uint16_t heart_beat_check_sec = NET_DEFAULT_HEARTBEAT_CHECK;
    uint32_t max_connections = NET_DEFAULT_MAX_CONN;
    if (lua_istable(L, 1))
    {
        int top = lua_gettop(L);
        lua_getfield(L, 1, "heart_beat");
        if (lua_isinteger(L, -1))
        {
            heart_beat_sec = (uint16_t)luaL_checkinteger(L, -1);
        }
        lua_getfield(L, 1, "heart_beat_check");
        if (lua_isinteger(L, -1))
        {
            heart_beat_check_sec = (uint16_t)luaL_checkinteger(L, -1);
        }
        lua_getfield(L, 1, "max_connection");
        if (lua_isinteger(L, -1))
        {
            max_connections = (uint16_t)luaL_checkinteger(L, -1);
        }
        lua_pop(L, lua_gettop(L) - top);
    }

    struct qsf_net_server_s* s = qsf_create_net_server(global_loop, max_connections,
        heart_beat_sec, heart_beat_check_sec);
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
        if (lua_pcall(L, 3, 0, 0) != LUA_OK)
        {
            qsf_log("%s\n", lua_tostring(L, -1));
        }
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
// net.client interface

static int create_client(lua_State* L)
{
    qsf_net_client_t* c = qsf_create_net_client(global_loop);
    net_client_t* client = lua_newuserdata(L, sizeof(net_client_t));
    client->c = c;
    client->L = L;
    client->connect_ref = LUA_NOREF;
    client->read_ref = LUA_NOREF;
    qsf_net_client_set_udata(c, client);
    luaL_getmetatable(L, CLIENT_HANDLE);
    lua_setmetatable(L, -2);
    return 1;
}

static int client_gc(lua_State* L)
{
    net_client_t* client = check_client(L);
    luaL_unref(L, LUA_REGISTRYINDEX, client->connect_ref);
    luaL_unref(L, LUA_REGISTRYINDEX, client->read_ref);
    qsf_net_client_close(client->c);
    return 0;
}

static int client_close(lua_State* L)
{
    net_client_t* client = check_client(L);
    qsf_net_client_shutdown(client->c);
    return 0;
}

static void on_client_connect(int error, void* ud)
{
    assert(ud);
    net_client_t* client = ud;
    int ref = client->connect_ref;
    lua_State* L = client->L;
    lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
    if (lua_isfunction(L, -1))
    {
        if (error == 0)
            lua_pushnil(L);
        else
            lua_pushinteger(L, error);
        if (lua_pcall(L, 1, 0, 0) != LUA_OK)
        {
            qsf_log("%s\n", lua_tostring(L, -1));
        }
        luaL_unref(L, LUA_REGISTRYINDEX, client->connect_ref);
        client->connect_ref = LUA_NOREF;
    }
}

static int client_connect(lua_State* L)
{
    net_client_t* client = check_client(L);
    luaL_unref(L, LUA_REGISTRYINDEX, client->connect_ref);
    const char* host = luaL_checkstring(L, 2);
    int port = (int)luaL_checkinteger(L, 3);
    luaL_argcheck(L, lua_isfunction(L, 4), 4, "connect callback must be function type");
    int connect_ref = luaL_ref(L, LUA_REGISTRYINDEX);
    qsf_assert(connect_ref != LUA_NOREF, "luaL_ref() failed.");
    client->connect_ref = connect_ref;
    qsf_net_client_connect(client->c, host, port, on_client_connect);
    return 0;
}

static void on_client_read(int error, const char* data, uint16_t size, void* ud)
{
    assert(data && size && ud);
    net_client_t* client = ud;
    int ref = client->read_ref;
    lua_State* L = client->L;
    lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
    if (lua_isfunction(L, -1))
    {
        if (error == 0)
        {
            lua_pushnil(L);
        }
        else
        {
            lua_pushinteger(L, error);
        }
        lua_pushlstring(L, data, size);
        if (lua_pcall(L, 2, 0, 0) != LUA_OK)
        {
            qsf_log("%s\n", lua_tostring(L, -1));
        }
    }
}

static int client_read(lua_State* L)
{
    net_client_t* client = check_client(L);
    luaL_unref(L, LUA_REGISTRYINDEX, client->read_ref);
    luaL_argcheck(L, lua_isfunction(L, 2), 2, "read callback must be function type");
    int read_ref = luaL_ref(L, LUA_REGISTRYINDEX);
    qsf_assert(read_ref != LUA_NOREF, "luaL_ref() failed.");
    client->read_ref = read_ref;
    qsf_net_client_read(client->c, on_client_read);
    return 0;
}

static int client_write(lua_State* L)
{
    net_client_t* client = check_client(L);
    size_t size;
    const char* data = luaL_checklstring(L, 2, &size);
    if (size <= UINT16_MAX)
    {
        qsf_net_client_write(client->c, data, (uint16_t)size);
    }
    else
    {
        luaL_error(L, "invalid client.write size: %d.", size);
    }
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

static void create_meta(lua_State* L, const char* name, const luaL_Reg* lib)
{
    assert(L && name && lib);
    luaL_newmetatable(L, name);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, lib, 0);
    lua_pop(L, 1);  /* pop new metatable */
}

static void make_meta(lua_State* L)
{
    static const luaL_Reg server_lib[] =
    {
        { "__gc", server_gc },
        { "start", server_start },
        { "stop", server_stop },
        { "write", server_write },
        { "broadcast", server_broadcast },
        { "shutdown", server_shutdown},
        { "kick", server_close },
        { "address_of", server_address_of},
        { "size", server_size },
        { NULL, NULL },
    };
    static const luaL_Reg client_lib[] =
    {
        { "__gc", client_gc },
        { "close", client_close },
        { "connect", client_connect },
        { "read", client_read },
        { "write", client_write },
        { NULL, NULL },
    };
    create_meta(L, SERVER_HANDLE, server_lib);
    create_meta(L, CLIENT_HANDLE, client_lib);
}

LUALIB_API int luaopen_net(lua_State* L)
{
    global_loop = uv_default_loop();
    static const luaL_Reg lib[] = 
    {
        { "create_server", create_server },
        { "create_client", create_client },
        { "poll", net_poll },
        { "stop", net_stop },
        {NULL, NULL}
    };
    luaL_newlib(L, lib);
    make_meta(L);
    return 1;
}
