// Copyright (C) 2014 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.


#include <assert.h>
#include <malloc.h>
#include <uv.h>
#include <lua.h>
#include <lauxlib.h>
#include "luv.h"

int luv_check_ref(lua_State* L, int idx)
{
    luaL_argcheck(L, lua_isfunction(L, idx), idx, "callback must be function");
    int ref = luaL_ref(L, LUA_REGISTRYINDEX);
    if (ref == LUA_NOREF || ref == LUA_REFNIL)
    {
        return luaL_error(L, "luaL_ref() failed: %d", ref);
    }
    return ref;
}

uv_loop_t* luv_loop(lua_State* L)
{
    lua_getfield(L, LUA_REGISTRYINDEX, "uv_loop");
    uv_loop_t* loop = (uv_loop_t*)lua_touserdata(L, -1);
    assert(loop != NULL);
    lua_pop(L, 1);
    return loop;
}

//////////////////////////////////////////////////////////////////////////

static int luv_run(lua_State* L)
{
    static const char* run_modes[] =
    {
        "default", "once", "nowait", NULL
    };
    uv_run_mode mode = (uv_run_mode)luaL_checkoption(L, 1, "default", run_modes);
    uv_loop_t* loop = luv_loop(L);
    int r = uv_run(loop, mode);
    return 0;
}

static int luv_stop(lua_State* L)
{
    uv_loop_t* loop = luv_loop(L);
    uv_stop(loop);
    return 0;
}

static int luv_close(lua_State* L)
{
    uv_loop_t* loop = luv_loop(L);
    int r = uv_loop_close(loop);
    if (r != 0)
        return luv_error(L, r);
    return 0;
}

static int luv_now(lua_State* L)
{
    uv_loop_t* loop = luv_loop(L);
    uint64_t now = uv_now(loop);
    lua_pushinteger(L, now);
    return 1;
}

int luaopen_uv(lua_State* L)
{
    static const luaL_Reg lib[] =
    { 
        { "new_tcp", luv_new_tcp },
        { "new_timer", luv_new_timer },
        { "run", luv_run },
        { "stop", luv_stop },
        { "close", luv_close },
        { "now", luv_now },
        { NULL, NULL },
    };

    lua_getfield(L, LUA_REGISTRYINDEX, "uv_loop");
    uv_loop_t* loop = (uv_loop_t*)lua_touserdata(L, -1);
    if (loop == NULL)
    {
        lua_pop(L, 1);
        loop = (uv_loop_t*)malloc(sizeof(uv_loop_t));
        uv_loop_init(loop);
        loop->data = L;
        lua_pushlightuserdata(L, loop);
        lua_setfield(L, LUA_REGISTRYINDEX, "uv_loop");
    }
    luaL_newlib(L, lib);
    luv_tcp_init(L);
    luv_timer_init(L);
    return 1;
}
