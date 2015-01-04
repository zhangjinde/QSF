#include <lua.hpp>
#include "core/random.h"


// a drop-in replacement for `math.random`
static int utils_random(lua_State *L)
{
    switch (lua_gettop(L))   /* check number of arguments */
    {
    case 1:   /* only upper limit */
    {
        lua_Integer upper = luaL_checkinteger(L, 1);
        luaL_argcheck(L, 1 <= upper, 1, "interval is empty");
        lua_pushinteger(L, Random::rand64(upper + 1));  /* [1, u] */
        break;
    }
    case 2:   /* lower and upper limits */
    {
        lua_Integer lower = luaL_checkinteger(L, 1);
        lua_Integer upper = luaL_checkinteger(L, 2);
        luaL_argcheck(L, lower <= upper, 2, "interval is empty");
        lua_pushinteger(L, Random::rand64(lower, upper + 1));  /* [l, u] */
        break;
    }
    default: 
        return luaL_error(L, "wrong number of arguments");
    }
    return 1;
}

extern "C"
int luaopen_utils(lua_State* L)
{
    static const luaL_Reg lib[] =
    {        
        { "random", utils_random },
        { NULL, NULL },
    };
    luaL_newlib(L, lib);
    return 1;
}
