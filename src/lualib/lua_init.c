// Copyright (C) 2014 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#include <lua.h>
#include <lauxlib.h>

#ifdef __cplusplus
extern "C" {
#endif

// declarations
int luaopen_mq(lua_State* L);
int luaopen_process(lua_State* L);
int luaopen_zmq(lua_State* L);
int luaopen_uuid(lua_State* L);
int luaopen_mysql(lua_State* L);
int luaopen_crypto(lua_State* L);
int luaopen_zlib(lua_State* L);
int luaopen_fs(lua_State* L);
int luaopen_cjson(lua_State *l);

#ifdef __cplusplus
}
#endif

void lua_initlibs(lua_State* L)
{
    static const luaL_Reg libs[] =
    {
        { "mq", luaopen_mq },
        { "zmq", luaopen_zmq },
        { "uuid", luaopen_uuid },
        { "crypto", luaopen_crypto },
        { "process", luaopen_process },
        { "mysql", luaopen_mysql },
        { "zlib", luaopen_zlib },
        { "fs", luaopen_fs },
        { "cjson", luaopen_cjson },
        { NULL, NULL },
    };

    // add open functions into 'package.preload' table
    luaL_getsubtable(L, LUA_REGISTRYINDEX, "_PRELOAD");
    for (const luaL_Reg *lib = libs; lib->func; lib++)
    {
        lua_pushcfunction(L, lib->func);
        lua_setfield(L, -2, lib->name);
    }
    lua_pop(L, 1);  /* remove _PRELOAD table */
}
