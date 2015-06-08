// Copyright (C) 2014-2015 chenqiang@chaoyuehudong.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#include "qsf_env.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include "qsf.h"

typedef struct qsf_env_s
{
    uv_mutex_t  mutex;
    lua_State*  L;
}qsf_env_t;

// global envrionment object, thread-safe access
static qsf_env_t  global_env;


const char* qsf_getenv(const char* key)
{
    lua_State* L = global_env.L;
    assert(L && key);
    uv_mutex_lock(&global_env.mutex);
    lua_getglobal(L, key);
    const char* s = lua_tostring(L, -1);
    lua_pop(L, 1);
    uv_mutex_unlock(&global_env.mutex);
    return s;
}

int64_t qsf_getenv_int(const char* key)
{
    const char* s = qsf_getenv(key);
    if (s)
    {
        return atoll(s);
    }
    return 0;
}

void qsf_setenv(const char* key, const char* value)
{
    lua_State* L = global_env.L;
    assert(L && key && value);
    uv_mutex_lock(&global_env.mutex);
    lua_getglobal(L, key);
    if (lua_isnil(L, -1))
    {
        lua_pushstring(L, value);
        lua_setglobal(L, key);
    }
    lua_pop(L, 1);
    uv_mutex_unlock(&global_env.mutex);
}

int qsf_env_init(const char* file)
{
    int r = uv_mutex_init(&global_env.mutex);
    if (r != 0)
    {
        qsf_log("env: uv_mutex_init() failed.\n");
        return r;
    }
    lua_State* L = luaL_newstate();
    luaL_openlibs(L);
    r = luaL_dofile(L, file);
    if (r != LUA_OK)
    {
        qsf_log("%s\n", lua_tostring(L, -1));
        lua_close(L);
        return r;
    }
    global_env.L = L;
    return 0;
}

void qsf_env_exit()
{
    uv_mutex_destroy(&global_env.mutex);
    lua_close(global_env.L);
    memset(&global_env, 0, sizeof(global_env));
}
