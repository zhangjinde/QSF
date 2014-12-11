// Copyright (C) 2014 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#include <assert.h>
#include <stdint.h>
#include <string>
#include <memory>
#include <typeinfo>
#include <exception>
#include <lua.hpp>
#include "net/gate.h"
#include "net/client.h"


static std::unique_ptr<boost::asio::io_service>   global_io_service;

struct Gateway
{
    std::unique_ptr<net::Gate>  server;
    int32_t ref;
};

struct Client
{
    std::unique_ptr<net::Client>  client;
    int32_t ref;
};


#define GATE_HANDLE     "gate*.gc"
#define CLIENT_HANDLE   "client*.gc"

#define check_gate(L)   (*(Gateway**)luaL_checkudata(L, 1, GATE_HANDLE))
#define check_client(L) (*(Client**)luaL_checkudata(L, 1, CLIENT_HANDLE))


//////////////////////////////////////////////////////////////////////////
//  gate interface

static int gate_create(lua_State* L)
{
    using namespace net;
    Gateway* ptr = new Gateway;
    assert(ptr);
    int top = lua_gettop(L);
    luaL_argcheck(L, top > 0 && lua_istable(L, 1), 1, "parameter must be table");
    lua_getfield(L, 1, "heart_beat");
    uint32_t heart_beat_sec = (uint32_t)luaL_optinteger(L, -1, 
        DEFAULT_MAX_HEARTBEAT_SEC);
    lua_getfield(L, 1, "heart_beat_check");
    uint32_t heart_beat_check_sec = (uint32_t)luaL_optinteger(L, -1, 
        DEFAULT_HEARTBEAT_CHECK_SEC);
    lua_getfield(L, 1, "max_connection");
    uint32_t max_connections = (uint32_t)luaL_optinteger(L, -1,
        DEFAULT_MAX_CONNECTIONS);
    lua_getfield(L, 1, "no_compression_size");
    uint32_t no_compression_size = (uint16_t)luaL_optinteger(L, -1,
        DEFAULT_NO_COMPRESSION_SIZE);
    lua_pop(L, lua_gettop(L) - top);

    ptr->server.reset(new Gate(*global_io_service, 
        max_connections, heart_beat_sec, heart_beat_check_sec, 
        no_compression_size));
    ptr->ref = LUA_NOREF;
    void* udata = lua_newuserdata(L, sizeof(ptr));
    memcpy(udata, &ptr, sizeof(ptr));
    luaL_getmetatable(L, GATE_HANDLE);
    lua_setmetatable(L, -2);
    return 1;
}

static int gate_stop(lua_State* L)
{
    Gateway* gate = check_gate(L);
    assert(gate);
    gate->server->stop();
    return 0;
}

static int gate_gc(lua_State* L)
{
    Gateway* gate = check_gate(L);
    gate_stop(L);
    luaL_unref(L, LUA_REGISTRYINDEX, gate->ref);
    delete gate;
    return 0;
}

static int gate_tostring(lua_State* L)
{
    Gateway* gate = check_gate(L);
    assert(gate);
    lua_pushfstring(L, "gate* (%p)", gate);
    return 1;
}

static int gate_start(lua_State* L)
{
    Gateway* gate = check_gate(L);
    assert(gate);
    const std::string& host = luaL_checkstring(L, 2);
    int16_t port = (int16_t)luaL_checkint(L, 3);
    luaL_argcheck(L, lua_isfunction(L, -1), 4, "callback must be function");
    int ref = luaL_ref(L, LUA_REGISTRYINDEX);
    if (ref == LUA_NOREF || ref == LUA_REFNIL)
    {
        return luaL_error(L, "luaL_ref() failed: %d", ref);
    }
    gate->ref = ref;
    gate->server->start(host, port, [=](int err, uint32_t serial, ByteRange data)
    {
        lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
        luaL_argcheck(L, lua_isfunction(L, -1), 1, "callback must be function");
        lua_pushinteger(L, err);
        lua_pushnumber(L, serial);
        lua_pushlstring(L, (const char*)data.data(), data.size());
        if (lua_pcall(L, 3, LUA_MULTRET, 0) != 0)
        {
            LOG(ERROR) << lua_tostring(L, -1);
        }
    });
    return 0;
}

static int gate_send(lua_State* L)
{
    Gateway* gate = check_gate(L);
    assert(gate);
    uint32_t serial = (uint32_t)luaL_checknumber(L, 2);
    size_t size;
    const char* data = luaL_checklstring(L, 3, &size);
    luaL_argcheck(L, data && size > 0, 3, "invalid data");
    gate->server->send(serial, data, size);
    return 0;
}

static int gate_sendall(lua_State* L)
{
    Gateway* gate = check_gate(L);
    assert(gate);
    size_t size;
    const char* data = luaL_checklstring(L, 2, &size);
    luaL_argcheck(L, data && size > 0, 2, "invalid data");
    gate->server->sendAll(data, size);
    return 0;
}

static int gate_kick(lua_State* L)
{
    Gateway* gate = check_gate(L);
    assert(gate);
    uint32_t serial = (uint32_t)luaL_checknumber(L, 2);
    gate->server->kick(serial);
    return 0;
}

static int gate_kickall(lua_State* L)
{
    Gateway* gate = check_gate(L);
    assert(gate);
    gate->server->kickAll();
    return 0;
}

static int gate_deny(lua_State* L)
{
    Gateway* gate = check_gate(L);
    assert(gate);
    const std::string& address = luaL_checkstring(L, 2);
    gate->server->denyAddress(address);
    return 0;
}

static int gate_allow(lua_State* L)
{
    Gateway* gate = check_gate(L);
    assert(gate);
    const std::string& address = luaL_checkstring(L, 2);
    gate->server->allowAddress(address);
    return 0;
}

//////////////////////////////////////////////////////////////////////////
//  client interface
//
static int client_create(lua_State* L)
{
    Client* ptr = new Client;
    luaL_argcheck(L, lua_gettop(L) > 0, 1, "arguments must be integer");
    uint32_t heart_beat = heart_beat = luaL_optinteger(L, 1,
        net::DEFAULT_MAX_HEARTBEAT_SEC);
    ptr->client.reset(new net::Client(*global_io_service, heart_beat));
    ptr->ref = LUA_NOREF;
    void* udata = lua_newuserdata(L, sizeof(ptr));
    memcpy(udata, &ptr, sizeof(ptr));
    luaL_getmetatable(L, CLIENT_HANDLE);
    lua_setmetatable(L, -2);
    return 1;
}

static int client_stop(lua_State* L)
{
    Client* ptr = check_client(L);
    assert(ptr);
    ptr->client->stop();
    return 0;
}

static int client_gc(lua_State* L)
{
    Client* ptr = check_client(L);
    assert(ptr);
    client_stop(L);
    luaL_unref(L, LUA_REGISTRYINDEX, ptr->ref);
    delete ptr;
    return 0;
}

static int client_tostring(lua_State* L)
{
    Client* ptr = check_client(L);
    assert(ptr);
    lua_pushfstring(L, "client* (%p)", ptr);
    return 1;
}

static int client_connect(lua_State* L)
{
    Client* ptr = check_client(L);
    assert(ptr);
    const std::string& host = luaL_checkstring(L, 2);
    uint16_t port = (uint16_t)luaL_checkint(L, 3);
    try
    {
        ptr->client->connect(host, port);
        lua_pushboolean(L, true);
        return 1;
    }
    catch (std::exception& ex)
    {
        lua_pushboolean(L, true);
        lua_pushstring(L, ex.what());
        return 2;
    }
}

static int client_asyn_connect(lua_State* L)
{
    Client* ptr = check_client(L);
    assert(ptr);
    const std::string& host = luaL_checkstring(L, 2);
    uint16_t port = (uint16_t)luaL_checkint(L, 3);
    luaL_argcheck(L, lua_isfunction(L, -1), 4, "connect callback must be function");
    int ref = luaL_ref(L, LUA_REGISTRYINDEX);
    ptr->client->connect(host, port, [=](const boost::system::error_code& ec)
    {
        lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
        luaL_argcheck(L, lua_isfunction(L, -1), 1, "callback must be function");
        int args = 1;
        if (!ec)
        {
            lua_pushboolean(L, true);
        }
        else
        {
            lua_pushboolean(L, false);
            lua_pushlstring(L, ec.message().c_str(), ec.message().size());
            args = 2;
        }
        if (lua_pcall(L, args, LUA_MULTRET, 0) != 0)
        {
            LOG(ERROR) << lua_tostring(L, -1);
        }
        luaL_unref(L, LUA_REGISTRYINDEX, ref);
    });
    return 0;
}

static int client_send(lua_State* L)
{
    Client* ptr = check_client(L);
    assert(ptr);
    size_t size;
    const char* data = luaL_checklstring(L, 2, &size);
    luaL_argcheck(L, data && size > 0, 2, "invalid data");
    ptr->client->send(data, size);
    return 0;
}

static int client_read(lua_State* L)
{
    Client* ptr = check_client(L);
    assert(ptr);
    luaL_argcheck(L, lua_isfunction(L, -1), 2, "callback must be function");
    int ref = luaL_ref(L, LUA_REGISTRYINDEX);
    if (ref == LUA_NOREF || ref == LUA_REFNIL)
    {
        return luaL_error(L, "luaL_ref() failed: %d", ref);
    }
    ptr->ref = ref;
    ptr->client->startRead([=](ByteRange data)
    {
        lua_rawgeti(L, LUA_REGISTRYINDEX, ptr->ref);
        luaL_argcheck(L, lua_isfunction(L, -1), 1, "callback should be function");
        lua_pushlstring(L, (const char*)data.data(), data.size());
        if (lua_pcall(L, 1, LUA_MULTRET, 0) != 0)
        {
            LOG(ERROR) << lua_tostring(L, -1);
        }
    });
    return 0;
}

//////////////////////////////////////////////////////////////////////////
// service interface
//
static int service_poll(lua_State* L)
{
    size_t result = 0;
    try
    {
        result = global_io_service->poll();
    }
    catch (std::exception& ex)
    {
        return luaL_error(L, "poll() failed, %s: %s", typeid(ex).name(), ex.what());
    }
    lua_pushinteger(L, result);
    return 1;
}

static int service_stop(lua_State* L)
{
    global_io_service->stop();
    return 0;
}

static void make_meta_gate(lua_State* L)
{
    static const luaL_Reg metalib[] =
    {
        { "__gc", gate_gc },
        { "__tostring", gate_tostring },
        { "start", gate_start },
        { "stop", gate_stop },
        { "send", gate_send },
        { "sendall", gate_sendall },
        { "kick", gate_kick },
        { "deny", gate_deny },
        { NULL, NULL },
    };
    luaL_newmetatable(L, GATE_HANDLE);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, metalib, 0);
    lua_pop(L, 1);  /* pop new metatable */
}

static void make_meta_client(lua_State* L)
{
    static const luaL_Reg metalib[] =
    {
        { "__gc", client_gc },
        { "__tostring", client_tostring },
        { "stop", client_stop },
        { "connect", client_connect },
        { "asyn_connect", client_asyn_connect },
        { "send", client_send },
        { "read", client_read },
        { NULL, NULL },
    };
    luaL_newmetatable(L, CLIENT_HANDLE);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, metalib, 0);
    lua_pop(L, 1);  /* pop new metatable */
}

extern "C" 
int luaopen_gate(lua_State* L)
{
    global_io_service.reset(new boost::asio::io_service());

    static const luaL_Reg lib[] = 
    {
        { "create_server", gate_create },
        { "create_client", client_create }, 
        { "poll", service_poll },
        { "stop", service_stop },
        {NULL, NULL},
    };
    luaL_newlib(L, lib);
    make_meta_gate(L);
    make_meta_client(L);
    return 1;
}
