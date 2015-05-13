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
#include <sys/time.h>
#include <sys/stat.h>
#include <uuid/uuid.h>
#include <unistd.h>
#include <time.h>
#else
#include <direct.h>
#include <Windows.h>
#include <Objbase.h>
#endif

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
#if defined(_WIN32)
    uint64_t tick = GetTickCount64();
#elif defined(__linux__)
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    uint64_t tick = (ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    uint64_t tick = (tv.tv_sec * 1000 + tv.tv_usec / 1000);
#endif
    lua_pushnumber(L, (lua_Number)tick);
    return 1;
}

static int process_hrtime(lua_State* L)
{
    lua_pushnumber(L, (lua_Number)uv_hrtime());
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
    unsigned int value = 0;
#ifdef _WIN32
    rand_s(&value);
#else
    int retry = 100;
    while (__builtin_ia32_rdrand32_step(&value) == 0)
    {
        if (--retry == 0)
            break;
    }
#endif
    lua_pushinteger(L, value);
    return 1;
}

static int process_new_uuid(lua_State* L)
{
    char out[40] = {'\0'};
#ifdef _WIN32
    GUID guid;
    CoCreateGuid(&guid);
    const char* fmt = "{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X}";
    sprintf_s(out, 40, fmt, guid.Data1, guid.Data2, guid.Data3,
        guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
        guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
#else
    uint8_t uuid[16];
    uuid_generate(uuid);
    uuid_unparse(uuid, out);
#endif
    lua_pushstring(L, out);
    return 1;
}

LUALIB_API int luaopen_process(lua_State* L)
{
    static const luaL_Reg lib[] =
    {
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
        { "new_uuid", process_new_uuid },
        { NULL, NULL },
    };
    luaL_register(L, "process", lib);
    return 1;
}
