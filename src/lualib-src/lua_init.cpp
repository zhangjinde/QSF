#include "lua_init.h"
#include <lua.hpp>

extern "C" {

int luaopen_qsf_c(lua_State* L);

}


void lua_initlibs(lua_State* L)
{
    static const luaL_Reg libs[] =
    {
        { "qsf.c", luaopen_qsf_c },
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
