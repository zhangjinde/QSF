// Copyright (C) 2014 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#include <assert.h>
#include <lua.h>
#include <lauxlib.h>
#include <uv.h>
#include "luv.h"

typedef struct
{
    uv_timer_t  handle;
    int         ref;
}luv_timer_t;


#define LUV_TIMER       "uv_timer*"
#define check_timer(L)  ((luv_timer_t*)luaL_checkudata(L, 1, LUV_TIMER))

int luv_new_timer(lua_State* L)
{
    uv_loop_t* loop = luv_loop(L);
    luv_timer_t* timer = (luv_timer_t*)lua_newuserdata(L, sizeof(luv_timer_t));
    int r = uv_timer_init(loop, &timer->handle);
    if (r < 0)
    {
        return luv_error(L, r);
    }
    timer->ref = LUA_NOREF;
    timer->handle.data = timer;
    luaL_getmetatable(L, LUV_TIMER);
    lua_setmetatable(L, -2);
    return 1;
}

static void timer_cb(uv_timer_t* handle)
{
    lua_State* L = (lua_State*)handle->loop->data;
    luv_timer_t* timer = (luv_timer_t*)handle->data;
    assert(L && timer);
    if (timer->ref != LUA_NOREF)
    {
        lua_rawgeti(L, LUA_REGISTRYINDEX, timer->ref);
        luaL_argcheck(L, lua_isfunction(L, -1), -1, "callback must be function");
        lua_call(L, 0, 0);
    }
}

static int luv_timer_gc(lua_State* L)
{
    luv_timer_t* timer = check_timer(L);
    luaL_unref(L, LUA_REGISTRYINDEX, timer->ref);
    uv_close((uv_handle_t*)&timer->handle, NULL);
    return 0;
}

static int luv_timer_start(lua_State* L)
{
    luv_timer_t* timer = check_timer(L);
    uint64_t timeout = luaL_checkinteger(L, 2);
    uint64_t repeat = luaL_checkinteger(L, 3);
    luaL_unref(L, LUA_REGISTRYINDEX, timer->ref);
    timer->ref = luv_check_ref(L, 4);
    int r = uv_timer_start(&timer->handle, timer_cb, timeout, repeat);
    if (r < 0)
    {
        return luv_error(L, r);
    }
    return 0;
}

static int luv_timer_stop(lua_State* L)
{
    luv_timer_t* timer = check_timer(L);
    int r = uv_timer_stop(&timer->handle);
    if (r < 0)
    {
        return luv_error(L, r);
    }
    return 0;
}

static int luv_timer_again(lua_State* L)
{
    luv_timer_t* timer = check_timer(L);
    int r = uv_timer_again(&timer->handle);
    if (r < 0)
    {
        return luv_error(L, r);
    }
    return 0;
}

static int luv_timer_set_repeat(lua_State* L)
{
    luv_timer_t* timer = check_timer(L);
    uint64_t repeat = luaL_checkinteger(L, 2);
    uv_timer_set_repeat(&timer->handle, repeat);
    return 0;
}

static int luv_timer_get_repeat(lua_State* L)
{
    luv_timer_t* timer = check_timer(L);
    uint64_t repeat = uv_timer_get_repeat(&timer->handle);
    lua_pushinteger(L, repeat);
    return 1;
}

int luv_timer_init(lua_State* L)
{
    static const luaL_Reg metalib[] =
    {
        { "__gc", luv_timer_gc },
        { "start", luv_timer_start },
        { "stop", luv_timer_stop }, 
        { "again", luv_timer_again },
        { "set_repeat", luv_timer_set_repeat },
        { "get_repeat", luv_timer_get_repeat },
        { NULL, NULL },
    };
    luaL_newmetatable(L, LUV_TIMER);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, metalib, 0);
    lua_pop(L, 1);  /* pop new metatable */
    return 1;
}
