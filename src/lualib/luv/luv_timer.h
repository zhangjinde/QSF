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


static void on_timer_close(uv_handle_t* handle)
{
    luv_timer_t* timer = (luv_timer_t*)handle->data;
    qsf_free(timer);
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
    int active = uv_is_active((uv_handle_t*)handle);
    if (active == 0)
    {
        luaL_unref(L, LUA_REGISTRYINDEX, timer->ref);
        timer->ref = LUA_NOREF;
        if (!uv_is_closing((uv_handle_t*)handle))
        {
            uv_close((uv_handle_t*)handle, on_timer_close);
        }
    }
}

static int luv_new_timer(lua_State* L) 
{
    uv_loop_t* loop = luv_loop(L);
    luv_timer_t* timer = qsf_malloc(sizeof(*timer));
    memset(timer, 0, sizeof(*timer));
    int r = uv_timer_init(loop, &timer->handle);
    if (r < 0)
    {
        qsf_free(timer);
        return luv_error(L, r);
    }
    uint64_t timeout = luaL_checkinteger(L, 1);
    uint64_t repeat = luaL_checkinteger(L, 2);
    luaL_argcheck(L, lua_isfunction(L, 3), 3, "timer callback must be function type");
    int ref = luaL_ref(L, LUA_REGISTRYINDEX);
    qsf_assert(ref != LUA_NOREF, "luaL_ref() failed.");
    timer->ref = ref;
    timer->handle.data = timer;
    r = uv_timer_start(&timer->handle, timeout_cb, timeout, repeat);
    if (r < 0)
    {
        luaL_unref(L, LUA_REGISTRYINDEX, timer->ref);
        qsf_free(timer);
        return luv_error(L, r);
    }
    return 0;
}
