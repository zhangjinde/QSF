// Copyright (C) 2014 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#include "LuaService.h"
#include <lua.hpp>
#include "Env.h"
#include "core/Strings.h"
#include "core/Logging.h"
#include "lualib/lua_init.h"

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

LuaService::LuaService(Context& ctx)
    : Service("LuaSandbox", ctx)
{
    luaVM_ = luaL_newstate();
    CHECK_NOTNULL(luaVM_);
    Initialize();
}


LuaService::~LuaService()
{
    if (luaVM_)
    {
        lua_close(luaVM_);
        luaVM_ = nullptr;
    }
}

void LuaService::Initialize()
{
    lua_State* L = luaVM_;
    auto& ctx = this->GetCtx();
    luaL_checkversion(L);
    lua_gc(L, LUA_GCSTOP, 0);  /* stop collector during initialization */
    luaL_openlibs(L);
    lua_initlibs(L);
    LoadLibPath();
    lua_pushlightuserdata(L, &ctx);
    lua_setfield(L, LUA_REGISTRYINDEX, "qsf_ctx");
    lua_gc(L, LUA_GCRESTART, 0);
}

void LuaService::LoadLibPath()
{
    lua_State* L = luaVM_;
    string path = Env::Get("lua_path");
    if (!path.empty())
    {
        auto chunk = stringPrintf("package.path = package.path .. ';' .. '%s'", 
            path.c_str());
        int err = luaL_dostring(L, chunk.c_str());
        CHECK(!err) << lua_tostring(L, -1);
    }
    string cpath = Env::Get("lua_cpath");
    if (!cpath.empty())
    {
        auto chunk = stringPrintf("package.cpath = package.cpath .. ';' .. '%s'", 
            cpath.c_str());
        int err = luaL_dostring(L, chunk.c_str());
        CHECK(!err) << lua_tostring(L, -1);
    }
}

int LuaService::Run(const std::vector<string>& args)
{
    if (args.empty())
    {
        return 1;
    }
    
    const string& id = this->GetCtx().name();
    const string& filename = args[0];
    lua_State* L = luaVM_;
    int r = luaL_loadfile(L, filename.c_str());
    if (r != LUA_OK)
    {
        fprintf(stderr, "%s: %s\n", id.c_str(), lua_tostring(L, -1));
        return 1;
    }
    lua_pushlstring(L, id.c_str(), id.size());
    for (auto i = 1U; i < args.size(); i++)
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
