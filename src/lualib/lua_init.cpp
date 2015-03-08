// Copyright (C) 2014 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#include <lua.hpp>
#include <cstdint>


// declarations
extern "C" {

int luaopen_mq(lua_State* L);
int luaopen_zmq(lua_State* L);
int luaopen_uuid(lua_State* L);
int luaopen_crypto(lua_State* L);
int luaopen_process(lua_State* L);
int luaopen_mysql(lua_State* L);
int luaopen_hiredis(lua_State* L);

}

static_assert(sizeof(lua_Integer) == sizeof(int64_t), "LUA_INT_LONGLONG not defiend");

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
        { "hiredis", luaopen_hiredis },
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
