// Copyright (C) 2014-2015 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#include <assert.h>
#include <string.h>
#include <math.h>
#include <lua.h>
#include <lauxlib.h>
#include "qsf.h"

// string.start_with
static int string_startwith(lua_State* L)
{
    size_t hlen, nlen;
    const char* haystack = luaL_checklstring(L, 1, &hlen);
    const char* needle = luaL_checklstring(L, 2, &nlen);
    if (hlen >= nlen)
    {
        int cmp = strncmp(haystack, needle, nlen);
        lua_pushboolean(L, cmp == 0);
        return 1;
    }
    return 0;
}

// string.ends_with
static int string_endswith(lua_State* L)
{
    size_t hlen, nlen;
    const char* haystack = luaL_checklstring(L, 1, &hlen);
    const char* needle = luaL_checklstring(L, 2, &nlen);
    if (hlen >= nlen)
    {
        int cmp = strncmp(haystack + hlen - nlen, needle, nlen);
        lua_pushboolean(L, cmp == 0);
        return 1;
    }
    return 0;
}

// math.round
static int math_round(lua_State* L)
{
    lua_Number f = luaL_checknumber(L, 1);
    lua_pushnumber(L, round(f));
    return 1;
}

#define REGISTER_LIB(mod, name, f)  \
        lua_getglobal(L, mod);      \
        lua_pushcfunction(L, f);    \
        lua_setfield(L, -2, name);


int hook_stdlib(lua_State* L)
{
    REGISTER_LIB("string", "start_with", string_startwith);
    REGISTER_LIB("string", "ends_with", string_endswith);
    REGISTER_LIB("math", "round", math_round);
    return 0;
}