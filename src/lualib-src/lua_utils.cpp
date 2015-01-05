#include <lua.hpp>
#include "core/random.h"
#include "utils/md5.h"
#include "utils/utf.h"

static int utils_os(lua_State* L)
{
#ifdef _WIN32
    lua_pushliteral(L, "windows");
#else
    lua_pushliteral(L, "linux");
#endif
    return 1;
}

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

static int utils_md5(lua_State* L)
{
    size_t len;
    const char* data = luaL_checklstring(L, 1, &len);
    char buffer[HASHSIZE];
    md5(data, len, buffer);
    lua_pushlstring(L, buffer, HASHSIZE);
    return 1;
}

static int utils_as_gbk(lua_State* L)
{
#ifdef _WIN32
    size_t len;
    const char* str = luaL_checklstring(L, 1, &len);
    std::string u8str(str, len);
    std::string strgkb = U8toA(u8str);
    lua_pushlstring(L, strgkb.c_str(), strgkb.length());
    return 1;
#else
    return 0;
#endif
}

extern "C"
int luaopen_utils(lua_State* L)
{
    static const luaL_Reg lib[] =
    {
        { "os", utils_os },
        { "random", utils_random },
        { "md5", utils_md5 },
        { "as_gbk", utils_as_gbk },
        { NULL, NULL },
    };
    luaL_newlib(L, lib);
    return 1;
}
