// Copyright (C) 2014-2015 chenqiang@chaoyuehudong.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#include <assert.h>
#include <string.h>
#include <lua.h>
#include <lauxlib.h>
#include <stdint.h>

#ifndef _WIN32
#include <sys/types.h>
#include <sys/stat.h>
#include <uuid/uuid.h>
#include <unistd.h>
#else
#include <Windows.h>
#include <objbase.h>
#endif

#include "qsf.h"


// Number of 100 nanosecond units from 1/1/1601 to 1/1/1970
#define EPOCH_BIAS  116444736000000000i64

// the number of milliseconds since Epoch, 1970-01-01 00:00:00 +0000 (UTC).
static int process_time(lua_State* L)
{
    uint64_t now = 0;
#ifdef _WIN32
    GetSystemTimeAsFileTime((FILETIME*)&now);
    now = (now - EPOCH_BIAS) / 10000;
#else
    struct timeval val = { 0, 0 };
    gettimeofday(&val, NULL);
    now = val.tv_sec * 1000 + val.tv_usec / 1000;
#endif
    lua_pushinteger(L, now);
    return 1;
}

static int process_sleep(lua_State* L)
{
#ifdef _WIN32
    DWORD msec = (DWORD)luaL_checkinteger(L, 1);
    Sleep(msec);
#else
    int msec = (int)luaL_checkinteger(L, 1);
    usleep(msec * 1000);
#endif
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
    // default is binary representation
    const char* option = luaL_optstring(L, 1, "bin"); 
#ifdef _WIN32
    GUID uuid;
    CoCreateGuid(&uuid);
#else
    uint8_t uuid[16];
    uuid_generate(uuid);
#endif
    if (memcmp(option, "hex", 3) == 0)
    {
        char out[40] = { '\0' };
#ifdef _WIN32
        const char* fmt = "%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X";
        sprintf_s(out, sizeof(out), fmt, uuid.Data1, uuid.Data2, uuid.Data3,
            uuid.Data4[0], uuid.Data4[1], uuid.Data4[2], uuid.Data4[3],
            uuid.Data4[4], uuid.Data4[5], uuid.Data4[6], uuid.Data4[7]);
#else 
        uuid_unparse(uuid, out);
#endif
        lua_pushlstring(L, out, 36);
    }
    else // binary
    {
        lua_pushlstring(L, (const char*)&uuid, sizeof(uuid));
    }
    return 1;
}

#define SET_FIELD(n,v)  (lua_pushliteral(L, n), lua_pushstring(L, v), lua_settable(L, -3))

LUALIB_API int luaopen_process(lua_State* L)
{
    static const luaL_Reg lib[] =
    {
        { "time", process_time },
        { "sleep", process_sleep },
        { "rand32", process_rand32 },
        { "createUUID", process_new_uuid },
        { NULL, NULL },
    };
    luaL_newlib(L, lib);
    SET_FIELD("platform", PLATFORM_STRING);
    return 1;
}
