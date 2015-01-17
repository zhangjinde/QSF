#include <lua.hpp>
#include <thread>
#include <chrono>
#include "core/Benchmark.h"     // getNowTickCount()
#include "core/Random.h"
#include "utils/MD5.h"
#include "utils/UTF.h"


static int process_sleep(lua_State* L)
{
    int msec = luaL_checkint(L, 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(msec));
    return 0;
}

static int process_gettick(lua_State* L)
{
    uint64_t ticks = getNowTickCount() / 100000UL;
    lua_pushnumber(L, (lua_Number)ticks);
    return 1;
}

// number of CPU core
static int process_concurrency(lua_State* L)
{
    lua_pushinteger(L, std::thread::hardware_concurrency());
    return 1;
}

static int process_os(lua_State* L)
{
#ifdef _WIN32
    lua_pushliteral(L, "windows");
#elif defined(__linux__)
    lua_pushliteral(L, "linux");
#else
#error platform not supported
#endif
    return 1;
}

// a drop-in replacement for `math.random`
static int process_random(lua_State *L)
{
    switch (lua_gettop(L))   /* check number of arguments */
    {
    case 1:   /* only upper limit */
    {
        uint32_t upper = (uint32_t)luaL_checkinteger(L, 1);
        luaL_argcheck(L, 1 <= upper, 1, "interval is empty");
        lua_pushinteger(L, Random::rand32(upper + 1));  /* [1, u] */
        break;
    }
    case 2:   /* lower and upper limits */
    {
        uint32_t lower = (uint32_t)luaL_checkinteger(L, 1);
        uint32_t upper = (uint32_t)luaL_checkinteger(L, 2);
        luaL_argcheck(L, lower <= upper, 2, "interval is empty");
        lua_pushinteger(L, Random::rand32(lower, upper + 1));  /* [l, u] */
        break;
    }
    default: 
        return luaL_error(L, "wrong number of arguments");
    }
    return 1;
}

static int process_md5(lua_State* L)
{
    size_t len;
    const char* data = luaL_checklstring(L, 1, &len);
    char buffer[HASHSIZE];
    md5(data, len, buffer);
    lua_pushlstring(L, buffer, HASHSIZE);
    return 1;
}

static int process_as_gbk(lua_State* L)
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

extern "C" int luaopen_process(lua_State* L)
{
    static const luaL_Reg lib[] =
    {
        { "sleep", process_sleep },
        { "gettick", process_gettick },
        { "concurrency", process_concurrency },
        { "os", process_os },
        { "random", process_random },
        { "md5", process_md5 },
        { "as_gbk", process_as_gbk },
        { NULL, NULL },
    };
    luaL_newlib(L, lib);
    return 1;
}
