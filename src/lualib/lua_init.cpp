#include "lua_init.h"
#include <lua.hpp>
#include <cstdint>


// declarations
extern "C" {

int luaopen_mq(lua_State* L);
int luaopen_process(lua_State* L);
int luaopen_luazmq(lua_State* L);
int luaopen_gate(lua_State* L);
int luaopen_uuid(lua_State* L);
int luaopen_luamysql(lua_State* L);
int luaopen_luahiredis(lua_State* L);

}

static_assert(sizeof(lua_Integer) == sizeof(int64_t), "LUA_INT_LONGLONG not defiend");

void lua_initlibs(lua_State* L)
{
    static const luaL_Reg libs[] =
    {
        { "luamq", luaopen_mq },
        { "process", luaopen_process },
        { "luazmq", luaopen_luazmq },
        { "luagate", luaopen_gate },
        { "luauuid", luaopen_uuid },
        { "luamysql", luaopen_luamysql },
        { "luahiredis", luaopen_luahiredis },
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
