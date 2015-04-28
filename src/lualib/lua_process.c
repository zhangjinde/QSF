// Copyright (C) 2014-2015 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#include <float.h>
#include <assert.h>
#include <string.h>
#include <lua.h>
#include <lauxlib.h>
#include <uv.h>
#include "qsf.h"

#ifndef _WIN32
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#else
#include <direct.h>
#include <Windows.h>
#endif

static int process_os(lua_State* L)
{
#if defined(_WIN32)
    lua_pushliteral(L, "windows");
#elif defined(__linux__)
    lua_pushliteral(L, "linux");
#else
#error platform not supported
#endif
    return 1;
}

static int process_sleep(lua_State* L)
{
    int msec = (int)luaL_checkinteger(L, 1);
#ifdef _WIN32 
    Sleep(msec);
#else
    usleep(msec * 1000);
#endif
    return 0;
}

static int process_gettick(lua_State* L)
{
#ifdef _WIN32
    int64_t tick = GetTickCount64();
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    int64_t tick = (ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
#endif
    lua_pushinteger(L, tick);
    return 1;
}

static int process_hrtime(lua_State* L)
{
    int64_t t = (int64_t)uv_hrtime();
    lua_pushinteger(L, t);
    return 1;
}

static int process_pid(lua_State* L)
{
#ifdef _WIN32
    int pid = GetCurrentProcessId();
#else
    int pid = getpid();
#endif
    lua_pushinteger(L, pid);
    return 1;
}

static int process_kill(lua_State* L)
{
    int pid = (int)luaL_checkinteger(L, 1);
    int sig = (int)luaL_checkinteger(L, 2);
    uv_kill(pid, sig);
    return 0;
}

static int process_free_memory(lua_State* L)
{
    int64_t bytes = uv_get_free_memory();
    lua_pushinteger(L, bytes);
    return 1;
}

static int process_total_memory(lua_State* L)
{
    int64_t bytes = uv_get_total_memory();
    lua_pushinteger(L, bytes);
    return 1;
}

static int process_set_title(lua_State* L)
{
    const char* title = luaL_checkstring(L, 1);
    uv_set_process_title(title);
    return 0;
}

static int process_exepath(lua_State* L)
{
    char buf[256] = { '\0' };
    size_t size = sizeof(buf);
    uv_exepath(buf, &size);
    lua_pushstring(L, buf);
    return 1;
}

static int process_cwd(lua_State* L)
{
    char buf[256] = { '\0' };
    size_t size = sizeof(buf);
    uv_cwd(buf, &size);
    lua_pushstring(L, buf);
    return 1;
}

static int process_chdir(lua_State* L)
{
    const char* dir = luaL_checkstring(L, 1);
    uv_chdir(dir);
    return 0;
}

static int process_rand32(lua_State* L)
{
    qsf_service_t* self = lua_touserdata(L, lua_upvalueindex(1));
    assert(self);
    lua_Integer low = 0, up = 0;
    switch (lua_gettop(L))
    {
    case 1: /* only upper limit */
        up = luaL_checkinteger(L, 1);
        break;
    case 2: /* lower and upper limits */
        low = luaL_checkinteger(L, 1);
        up = luaL_checkinteger(L, 2);
        break;
    default:
        return luaL_error(L, "wrong number of arguments");
    }
    /* random integer in the interval [low, up] */
    luaL_argcheck(L, low <= up, 1, "interval is empty");
    luaL_argcheck(L, low >= 0 || up <= UINT32_MAX + low, 1, "interval too large");
    lua_Integer r = qsf_service_rand32(self, (uint32_t)(up - low + 1));
    lua_pushinteger(L, r + low);
    return 1;
}

static int process_randf(lua_State* L)
{
    qsf_service_t* self = lua_touserdata(L, lua_upvalueindex(1));
    assert(self);
    lua_Number low = 0, up = 0;
    float r = qsf_service_randf(self);
    switch (lua_gettop(L))
    {
    case 0:
        lua_pushnumber(L, r);
        return 1;
    case 1: /* only upper limit */
        up = luaL_checknumber(L, 1);
        break;
    case 2: /* lower and upper limits */
        low = luaL_checknumber(L, 1);
        up = luaL_checknumber(L, 2);
        break;
    default:
        return luaL_error(L, "wrong number of arguments");
    }
    /* random float number in the interval (low, up) */
    luaL_argcheck(L, low <= up, 1, "interval is empty");
    luaL_argcheck(L, low >= 0 || up <= FLT_MAX + low, 1, "interval too large");
    lua_pushnumber(L, (r * (up - low) + low));
    return 1;
}

LUALIB_API int luaopen_process(lua_State* L)
{
    static const luaL_Reg lib[] =
    {
        { "os", process_os },
        { "sleep", process_sleep }, 
        { "gettick", process_gettick }, 
        { "hrtime", process_hrtime },
        { "pid", process_pid },
        { "kill", process_kill },
        { "free_memory", process_free_memory }, 
        { "total_memory", process_total_memory },
        { "set_title", process_set_title },
        { "exepath", process_exepath },
        { "cwd", process_cwd },
        { "chdir", process_chdir },
        { "rand32", process_rand32 },
        { "randf", process_randf },
        { NULL, NULL },
    };
    luaL_newlibtable(L, lib);
    lua_getfield(L, LUA_REGISTRYINDEX, "mq_ctx");
    struct qsf_service_s* self = lua_touserdata(L, -1);
    luaL_argcheck(L, self != NULL, 1, "invalid context pointer");
    luaL_setfuncs(L, lib, 1);
    return 1;
}
