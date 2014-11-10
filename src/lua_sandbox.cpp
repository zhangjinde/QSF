// Copyright (C) 2014 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#include "lua_sandbox.h"
#include <lua.hpp>
#include "core/strings.h"
#include "core/logging.h"
#include "env.h"
#include "lualib-src/lua_init.h"

using std::string;

static int lua_traceback(lua_State *L)
{
    if (!lua_isstring(L, 1))  /* Non-string error object? Try metamethod. */
    {
        if (lua_isnoneornil(L, 1) ||
            !luaL_callmeta(L, 1, "__tostring") ||
            !lua_isstring(L, -1))
            return 1;  /* Return non-string error object. */
        lua_remove(L, 1);  /* Replace object by result of __tostring metamethod. */
    }
    luaL_traceback(L, L, lua_tostring(L, 1), 1);
    return 1;
}

//////////////////////////////////////////////////////////////////////////

LuaSandBox::LuaSandBox(Context& ctx)
    : Service("luasandbox", ctx)
{
    L = luaL_newstate();
    CHECK_NOTNULL(L);
    initialize();
}


LuaSandBox::~LuaSandBox()
{
    if (L)
    {
        lua_close(L);
        L = nullptr;
    }
}

void LuaSandBox::initialize()
{
    auto& ctx = this->context();
    luaL_checkversion(L);
    lua_gc(L, LUA_GCSTOP, 0);  /* stop collector during initialization */
    luaL_openlibs(L);
    lua_initlibs(L);
    loadLibPath();
    lua_pushlightuserdata(L, &ctx);
    lua_setfield(L, LUA_REGISTRYINDEX, "qsf_ctx");
    lua_gc(L, LUA_GCRESTART, 0);
}

void LuaSandBox::loadLibPath()
{
    string path = Env::get("lua_path");
    if (!path.empty())
    {
        auto chunk = stringPrintf("package.path = package.path .. ';' .. '%s'", 
            path.c_str());
        int err = luaL_dostring(L, chunk.c_str());
        CHECK(!err) << lua_tostring(L, -1);
    }
    string cpath = Env::get("lua_cpath");
    if (!cpath.empty())
    {
        auto chunk = stringPrintf("package.cpath = package.cpath .. ';' .. '%s'", 
            cpath.c_str());
        int err = luaL_dostring(L, chunk.c_str());
        CHECK(!err) << lua_tostring(L, -1);
    }
}

int LuaSandBox::run(const std::vector<string>& args)
{
    assert(!args.empty());
    
    const string& id = this->context().name();
    const string& filename = args[0];

    int r = luaL_loadfile(L, filename.c_str());
    if (r != LUA_OK)
    {
        fprintf(stderr, "%s: %s\n", id.c_str(), lua_tostring(L, -1));
        return 1;
    }
    lua_pushlstring(L, id.c_str(), id.size());
    for (int i = 1; i < args.size(); i++)
    {
        lua_pushlstring(L, args[i].c_str(), args[i].size());
    }
    r = lua_pcall(L, (int)args.size(), 0, 0);
    if (r != LUA_OK)
    {
        fprintf(stderr, "%s: %s\n", id.c_str(), lua_tostring(L, -1));
        return 1;
    }

    return 0;
}
