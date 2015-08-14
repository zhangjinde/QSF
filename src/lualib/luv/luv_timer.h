// Copyright (C) 2014-2015 chenqiang@chaoyuehudong.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#include "luv.h"
#include <lua.h>
#include <lauxlib.h>
#include "qsf.h"


#define LUV_TIMER           "luv_timer*"
#define check_timer(L, idx) (luv_timer_t*)luaL_checkudata(L, idx, LUV_TIMER)


typedef struct luv_timer_s
{
    uv_timer_t  handle;
    int         ref;
}luv_timer_t;


static int luv_new_timer(lua_State* L) 
{
    uv_loop_t* loop = luv_loop(L);
    luv_timer_t* timer = lua_newuserdata(L, sizeof(*timer));
    int r = uv_timer_init(loop, &timer->handle);
    if (r < 0) 
    {
        lua_pop(L, 1);
        return luv_error(L, r);
    }
    timer->ref = LUA_NOREF;
    timer->handle.data = timer;
    luaL_getmetatable(L, LUV_TIMER);
    lua_setmetatable(L, -2);
    return 1;
}

static int luv_timer_gc(lua_State* L)
{
    luv_timer_t* timer = check_timer(L, 1);
    uv_handle_t* handle = (uv_handle_t*)&timer->handle;
    if (!uv_is_closing(handle))
    {
        uv_close(handle, NULL);
    }
    luaL_unref(L, LUA_REGISTRYINDEX, timer->ref);
    return 0;
}

static void timeout_cb(uv_timer_t* handle) 
{
    lua_State* L = luv_state(handle->loop);
    luv_timer_t* timer = (luv_timer_t*)handle->data;
    lua_rawgeti(L, LUA_REGISTRYINDEX, timer->ref);
    if (lua_isfunction(L, -1))
    {
        qsf_trace_pcall(L, 0);
    }
}

static int luv_timer_start(lua_State* L) 
{
    luv_timer_t* timer = check_timer(L, 1);
    uint64_t timeout = luaL_checkinteger(L, 2);
    uint64_t repeat = luaL_checkinteger(L, 3);
    luaL_argcheck(L, lua_isfunction(L, 4), 4, "timer callback must be function type");
    int ref = luaL_ref(L, LUA_REGISTRYINDEX);
    qsf_assert(ref != LUA_NOREF, "luaL_ref() failed.");
    luaL_unref(L, LUA_REGISTRYINDEX, timer->ref);
    timer->ref = ref;
    int r = uv_timer_start(&timer->handle, timeout_cb, timeout, repeat);
    if (r < 0)
    {
        luaL_unref(L, LUA_REGISTRYINDEX, timer->ref);
        timer->ref = LUA_NOREF;
        return luv_error(L, r);
    }
    return 0;
}

static int luv_timer_stop(lua_State* L) 
{
    luv_timer_t* timer = check_timer(L, 1);
    int r = uv_timer_stop(&timer->handle);
    if (r < 0)
    {
        return luv_error(L, r);
    }
    return 0;
}

static int luv_timer_again(lua_State* L) 
{
    luv_timer_t* timer = check_timer(L, 1);
    int r = uv_timer_again(&timer->handle);
    if (r < 0)
    {
        return luv_error(L, r);
    }
    return 0;
}

static int luv_timer_set_repeat(lua_State* L) 
{
    luv_timer_t* timer = check_timer(L, 1);
    uint64_t repeat = luaL_checkinteger(L, 2);
    uv_timer_set_repeat(&timer->handle, repeat);
    return 0;
}

static int luv_timer_get_repeat(lua_State* L) 
{
    luv_timer_t* timer = check_timer(L, 1);
    uint64_t repeat = uv_timer_get_repeat(&timer->handle);
    lua_pushinteger(L, repeat);
    return 1;
}

static int create_timer_meta(lua_State* L)
{
    static const luaL_Reg methods[] =
    {
        { "__gc", luv_timer_gc },
        { "start", luv_timer_start },
        { "stop", luv_timer_stop },
        { "again", luv_timer_again },
        { "setRepeat", luv_timer_set_repeat },
        { "getRepeat", luv_timer_get_repeat },
        { NULL, NULL }
    };
    luaL_newmetatable(L, LUV_TIMER);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, methods, 0);
    lua_pushliteral(L, "__metatable");
    lua_pushliteral(L, "cannot access this metatable");
    lua_settable(L, -3);
    lua_pop(L, 1);  /* pop new metatable */
    return 0;
}
