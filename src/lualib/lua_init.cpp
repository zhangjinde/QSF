#include "lua_init.h"
#include <lua.hpp>


// declarations
extern "C" {

int luaopen_qsf(lua_State* L);
int luaopen_utils(lua_State* L);
int luaopen_luazmq(lua_State* L);
int luaopen_gate(lua_State* L);
int luaopen_uuid(lua_State* L);
int luaopen_cmsgpack(lua_State* L);
int luaopen_cmsgpack_safe(lua_State* L);
}

void lua_initlibs(lua_State* L)
{
    static const luaL_Reg libs[] =
    {
        { "qsf", luaopen_qsf },
        { "qsf.utils", luaopen_utils },
        { "luazmq", luaopen_luazmq },
        { "gate", luaopen_gate },
        { "uuid", luaopen_uuid },
        { "cmsgpack", luaopen_cmsgpack },
        { "cmsgpack.safe", luaopen_cmsgpack_safe },
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
