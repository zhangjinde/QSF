#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <lua.hpp>
#include "core/benchmark.h"     // getNowTickCount()
#include "service/context.h"
#include "qsf.h"


// send message to a named service
static int qsf_send(lua_State* L)
{
    Context* self = (Context*)lua_touserdata(L, lua_upvalueindex(1));
    assert(self);
    size_t name_size = 0;
    size_t data_size = 0;
    const char* name = luaL_checklstring(L, 1, &name_size);
    const char* data = luaL_checklstring(L, 2, &data_size);
    assert(data && data_size && name && name_size && name_size <= MAX_NAME_SIZE);
    self->send(StringPiece(name, name_size), StringPiece(data, data_size));
    return 0;
}

// recv new message from another service
static int qsf_recv(lua_State* L)
{
    Context* self = (Context*)lua_touserdata(L, lua_upvalueindex(1));
    assert(self);
    bool dontwait = false;
    if (lua_gettop(L) > 0)
    {
        const char* option = luaL_checkstring(L, -1);
        dontwait = (strcmp(option, "dontwait") == 0);
    }
    int r = 0;
    self->recv([&](StringPiece name, StringPiece data)
    {
        if (data.size() > 0)
        {
            lua_pushlstring(L, name.data(), name.size());
            lua_pushlstring(L, data.data(), data.size());
            r = 2;
        }
    }, dontwait);

    return r;
}

// launch a new service
static int qsf_launch(lua_State* L)
{
    std::string type = luaL_checkstring(L, 1);
    std::string ident = luaL_checkstring(L, 2);
    std::string args = luaL_checkstring(L, 3);
    int top = lua_gettop(L);
    for (int i = 4; i <= top; i++)
    {
        const char* value = luaL_checkstring(L, i);
        args.append(" ");
        args.append(value);
    }
    bool r = qsf::createService(type, ident, args);
    lua_pushboolean(L, r);
    return 1;
}

// stop all services
static int qsf_shutdown(lua_State* L)
{
    qsf::stop();
    return 0;
}

static int qsf_sleep(lua_State* L)
{
    int msec = luaL_checkint(L, 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(msec));
    return 0;
}

static int qsf_tickcount(lua_State* L)
{
    uint64_t ticks = getNowTickCount() / 100000UL;
    lua_pushnumber(L, (lua_Number)ticks);
    return 1;
}

// number of CPU core
static int qsf_concurrency(lua_State* L)
{
    lua_pushinteger(L, std::thread::hardware_concurrency());
    return 1;
}

extern "C" 
int luaopen_qsf_c(lua_State* L)
{
    static const luaL_Reg lib[] = 
    {
        { "send", qsf_send },
        { "recv", qsf_recv },
        { "launch", qsf_launch },
        { "shutdown", qsf_shutdown },
        { "sleep", qsf_sleep },
        { "tickcount", qsf_tickcount },
        { "concurrency", qsf_concurrency },
        {NULL, NULL},
    };
    luaL_newlibtable(L, lib);
    lua_getfield(L, LUA_REGISTRYINDEX, "qsf_ctx");
    Context* self = (Context*)lua_touserdata(L, -1);
    if (self == nullptr)
    {
        return luaL_error(L, "Context pointer is null");
    }
    luaL_setfuncs(L, lib, 1);
    return 1;
}
