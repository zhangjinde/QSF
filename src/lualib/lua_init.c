// Copyright (C) 2014-2015 chenqiang@chaoyuehudong.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#include <lua.h>
#include <lauxlib.h>
#include "qsf.h"



/* forward declarations */
extern int luaopen_mq(lua_State* L);
extern int luaopen_net(lua_State* L);
extern int luaopen_zmq(lua_State* L);
extern int luaopen_process(lua_State* L);
extern int luaopen_lfs(lua_State* L);
extern int luaopen_zlib(lua_State* L); 
extern int luaopen_mysql(lua_State* L);
extern int luaopen_crypto(lua_State* L); 

void init_preload_libs(lua_State* L)
{
    static const luaL_Reg libs[] =
    {
        { "mq", luaopen_mq },
        { "lfs", luaopen_lfs },
        { "net", luaopen_net },
        { "zmq", luaopen_zmq },
        { "zlib", luaopen_zlib },
        { "mysql", luaopen_mysql },
        { "crypto", luaopen_crypto },
        { "process", luaopen_process },
        { NULL, NULL },
    };

    /* add open functions into 'package.preload' table */
    luaL_getsubtable(L, LUA_REGISTRYINDEX, "_PRELOAD");
    for (const luaL_Reg *lib = libs; lib->func; lib++)
    {
        lua_pushcfunction(L, lib->func);
        lua_setfield(L, -2, lib->name);
    }
    lua_pop(L, 1);  /* remove _PRELOAD table */
}

static int ltraceback(lua_State* L)
{
    if (!lua_isstring(L, 1))   /* Non-string error object? Try metamethod. */
    {
        if (lua_isnoneornil(L, 1) ||
            !luaL_callmeta(L, 1, "__tostring") ||
            !lua_isstring(L, -1))
            return 1;  /* Return non-string error object. */
        lua_remove(L, 1);  /* Replace object by result of __tostring metamethod. */
    }
    luaL_traceback(L, L, lua_tostring(L, 1), 1);
    return 1;
}

int lua_trace_call(lua_State* L, int narg)
{
    int base = lua_gettop(L) - narg;  /* function index */
    lua_pushcfunction(L, ltraceback);  /* push traceback function */
    lua_insert(L, base);  /* put it under chunk and args */
    int r = lua_pcall(L, narg, 0, base);
    lua_remove(L, base);  /* remove traceback function */
    if (r != 0)
    {
        qsf_log("%s\n", lua_tostring(L, -1));
    }
    return r;
}
