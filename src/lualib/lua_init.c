// Copyright (C) 2014-2015 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#include <lua.h>
#include <lauxlib.h>


// forward declarations
int luaopen_mq(lua_State* L);
int luaopen_net(lua_State* L);
int luaopen_zmq(lua_State* L);
int luaopen_uuid(lua_State* L);
int luaopen_crypto(lua_State* L);
int luaopen_zlib(lua_State* L);
int luaopen_process(lua_State* L);
int luaopen_lfs(lua_State* L);

int hook_stdlib(lua_State* L);

void lua_initlibs(lua_State* L)
{
    static const luaL_Reg libs[] =
    {
        { "mq", luaopen_mq },
        { "lfs", luaopen_lfs },
        { "net", luaopen_net },
        { "zmq", luaopen_zmq },
        { "uuid", luaopen_uuid },
        { "zlib", luaopen_zlib },
        { "crypto", luaopen_crypto }, 
        { "process", luaopen_process },
        { NULL, NULL },
    };

    for (const luaL_Reg* lib = libs; lib->func; lib++) 
    {
        lua_pushcfunction(L, lib->func);
        lua_pushstring(L, lib->name);
        lua_call(L, 1, 0);
    }
    hook_stdlib(L);
}
