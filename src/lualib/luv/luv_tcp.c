// Copyright (C) 2014-2015 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#include <assert.h>
#include <lua.h>
#include <lauxlib.h>
#include <uv.h>
#include "luv.h"

typedef struct
{
    uv_tcp_t    handle;
    int         connection_cb;
    int         close_cb;
    int         write_cb;
    int         read_cb;
    char        buf[1];
}luv_tcp_t;

#define LUV_TCP         "luv_tcp_t*"
#define check_tcp(L)    ((luv_tcp_t*)luaL_checkudata(L, 1, LUV_TCP))

static luv_tcp_t* new_tcp_stream(lua_State* L, uv_loop_t* loop)
{
    luv_tcp_t* stream = (luv_tcp_t*)lua_newuserdata(L, sizeof(luv_tcp_t) + DEFAULT_BUF_SIZE);
    int r = uv_tcp_init(loop, &stream->handle);
    if (r < 0)
    {
        return NULL;
    }
    stream->connection_cb = LUA_NOREF;
    stream->close_cb = LUA_NOREF;
    stream->write_cb = LUA_NOREF;
    stream->read_cb = LUA_NOREF;
    stream->handle.data = stream;
    luaL_getmetatable(L, LUV_TCP);
    lua_setmetatable(L, -2);
    return stream;
}

int luv_new_tcp(lua_State* L)
{
    uv_loop_t* loop = luv_loop(L);
    if (new_tcp_stream(L, loop) == NULL)
    {
        return luaL_error(L, "create tcp_stream failed.");
    }
    return 1;
}

static int luv_tcp_gc(lua_State* L)
{
    luv_tcp_t* stream = check_tcp(L);
    luaL_unref(L, LUA_REGISTRYINDEX, stream->connection_cb);
    luaL_unref(L, LUA_REGISTRYINDEX, stream->write_cb);
    luaL_unref(L, LUA_REGISTRYINDEX, stream->read_cb);
    luaL_unref(L, LUA_REGISTRYINDEX, stream->close_cb);
    uv_handle_t* handle = (uv_handle_t*)&stream->handle;
    if (uv_is_closing(handle))
    {
        uv_close(handle, NULL);
    }
    return 0;
}

static int luv_tcp_close(lua_State* L)
{
    luv_tcp_t* stream = check_tcp(L);
    uv_handle_t* handle = (uv_handle_t*)&stream->handle;
    if (uv_is_closing(handle))
    {
        uv_close(handle, NULL);
    }
    return 0;
}

// IPv4 only
static int luv_tcp_bind(lua_State* L)
{
    luv_tcp_t* stream = check_tcp(L);
    const char* address = luaL_checkstring(L, 2);
    int port = (int)luaL_checkinteger(L, 3);
    struct sockaddr_in addr;
    int r = uv_ip4_addr(address, port, &addr);
    if (r < 0)
    {
        return luv_error(L, r);
    }
    r = uv_tcp_bind(&stream->handle, (const struct sockaddr*)&addr, 0);
    if (r < 0)
    {
        return luv_error(L, r);
    }
    return 0;
}

static void connection_cb(uv_stream_t* handle, int err)
{
    lua_State* L = (lua_State*)handle->loop->data;
    luv_tcp_t* stream = (luv_tcp_t*)handle->data;
    assert(L && stream);
    int ref = stream->connection_cb;
    if (ref != LUA_NOREF)
    {
        lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
        luaL_argcheck(L, lua_isfunction(L, -1), -1, "callback must be function");
        lua_pushinteger(L, err);
        lua_call(L, 1, 0);
    }
}

static int luv_tcp_listen(lua_State* L)
{
    uv_loop_t* loop = luv_loop(L);
    luv_tcp_t* stream = check_tcp(L);
    int backlog = (int)luaL_checkinteger(L, 2);
    luaL_unref(L, LUA_REGISTRYINDEX, stream->connection_cb);
    stream->connection_cb = luv_check_ref(L, 3);
    int r = uv_listen((uv_stream_t*)&stream->handle, backlog, connection_cb);
    if (r < 0)
    {
        return luv_error(L, r);
    }
    return 0;
}

static int luv_tcp_accept(lua_State* L)
{
    luv_tcp_t* server = check_tcp(L);
    luv_tcp_t* client = new_tcp_stream(L, server->handle.loop);
    int r = uv_accept((uv_stream_t*)&server->handle, (uv_stream_t*)&client->handle);
    if (r < 0)
    {
        return luv_error(L, r);
    }
    return 1;
}

static void alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf)
{
    luv_tcp_t* stream = (luv_tcp_t*)handle->data;
    assert(stream);
    buf->base = stream->buf;
    buf->len = sizeof(stream->buf);
}

static void read_cb(uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf)
{
    lua_State* L = (lua_State*)handle->loop->data;
    luv_tcp_t* stream = (luv_tcp_t*)handle->data;
    assert(L && stream);
    int ref = stream->read_cb;
    if (ref != LUA_NOREF)
    {
        lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
        luaL_argcheck(L, lua_isfunction(L, -1), -1, "callback must be function");
        if (nread > 0)
        {
            lua_pushnil(L);
            lua_pushlstring(L, buf->base, nread);
            lua_call(L, 2, 0);
        }
        else if (nread < 0)
        {
            lua_pushinteger(L, nread);
            lua_pushstring(L, uv_strerror(nread));
        }
    }
}

static int luv_read_start(lua_State* L)
{
    luv_tcp_t* stream = check_tcp(L);
    luaL_unref(L, LUA_REGISTRYINDEX, stream->read_cb);
    stream->read_cb = luv_check_ref(L, 2);
    int r = uv_read_start((uv_stream_t*)&stream->handle, alloc_cb, read_cb);
    if (r < 0)
    {
        return luv_error(L, r);
    }
    return 0;
}

static int luv_read_stop(lua_State* L)
{
    luv_tcp_t* stream = check_tcp(L);
    int r = uv_read_stop((uv_stream_t*)&stream->handle);
    if (r < 0)
    {
        return luv_error(L, r);
    }
    return 0;
}

static int luv_write(lua_State* L)
{
    luv_tcp_t* stream = check_tcp(L);
}

int luv_tcp_init(lua_State* L)
{
    static const luaL_Reg metalib[] =
    {
        { "__gc", luv_tcp_gc },
        { "close", luv_tcp_close },
        { "bind", luv_tcp_bind },
        { "listen", luv_tcp_listen },
        { "accept", luv_tcp_accept },
        { "read_start", luv_read_start },
        { "read_stop", luv_read_stop },
        { NULL, NULL },
    };
    luaL_newmetatable(L, LUV_TCP);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, metalib, 0);
    lua_pop(L, 1);  /* pop new metatable */
    return 1;
}
