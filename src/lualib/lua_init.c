// Copyright (C) 2014-2015 chenqiang@chaoyuehudong.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#include <lua.h>
#include <lauxlib.h>
#include "qsf.h"


// forward declarations
extern int luaopen_node(lua_State* L);
extern int luaopen_zmq(lua_State* L);
extern int luaopen_process(lua_State* L);
extern int luaopen_zlib(lua_State* L); 
extern int luaopen_mysql(lua_State* L);
extern int luaopen_crypto(lua_State* L);
extern int luaopen_net(lua_State* L);
extern int luaopen_luv(lua_State *L);
extern int luaopen_base64(lua_State* L);
extern int luaopen_lfs(lua_State *L);

static const luaL_Reg preload_libs[] =
{
    { "luv", luaopen_luv },
    { "zmq", luaopen_zmq },
    { "net", luaopen_net },
    { "lfs", luaopen_lfs },
    { "node", luaopen_node },
    { "zlib", luaopen_zlib },
    { "mysql", luaopen_mysql },
    { "crypto", luaopen_crypto },
    { "base64", luaopen_base64 },
    { "process", luaopen_process },
    { NULL, NULL },
};

void open_preload_libs(lua_State* L)
{
    // add open functions into 'package.preload' table
    luaL_getsubtable(L, LUA_REGISTRYINDEX, "_PRELOAD");
    for (const luaL_Reg *lib = preload_libs; lib->func; lib++)
    {
        lua_pushcfunction(L, lib->func);
        lua_setfield(L, -2, lib->name);
    }
    lua_pop(L, 1);  // remove _PRELOAD table
}
