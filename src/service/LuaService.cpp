// Copyright (C) 2014 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#include "LuaService.h"
#include <lua.hpp>
#include "core/Conv.h"
#include "core/Logging.h"
#include "Env.h"


using std::string;

extern "C" void lua_initlibs(lua_State* L);

// LUA_INTEGER must be 64-bit
static_assert(sizeof(LUA_INTEGER) == sizeof(int64_t), "LUA_INT_LONGLONG not defiend");

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
    lua_setfield(L, LUA_REGISTRYINDEX, "mq_ctx");
    lua_gc(L, LUA_GCRESTART, 0);
}

void LuaService::LoadLibPath()
{
    lua_State* L = luaVM_;
    string path = Env::Get("lua_path");
    if (!path.empty())
    {
        auto chunk = to<string>("package.path = package.path .. ';' .. '", path, "'");
        int err = luaL_dostring(L, chunk.c_str());
        CHECK(!err) << lua_tostring(L, -1);
    }
    string cpath = Env::Get("lua_cpath");
    if (!cpath.empty())
    {
        auto chunk = to<string>("package.cpath = package.cpath .. ';' .. '", cpath, "'");
        int err = luaL_dostring(L, chunk.c_str());
        CHECK(!err) << lua_tostring(L, -1);
    }
}

int LuaService::Run(const std::string& args)
{
    if (args.empty())
    {
        return 1;
    }
    const string& id = this->GetCtx().Name();
    string filename = args;
    string loader = "sys";
    size_t pos = args.find(" ");
    if (pos > 0 && pos != string::npos)
    {
        filename = args.substr(0, pos);
        loader = args.substr(pos);
    }
    
    lua_State* L = luaVM_;
    int r = luaL_loadfile(L, filename.c_str());
    if (r != LUA_OK)
    {
        fprintf(stderr, "%s\n", id.c_str(), lua_tostring(L, -1));
        return 1;
    }
    lua_pushlstring(L, loader.c_str(), loader.size());
    r = lua_pcall(L, 1, 0, 0);
    if (r != LUA_OK)
    {
        fprintf(stderr, "%s: %s\n", id.c_str(), lua_tostring(L, -1));
        return 1;
    }

    return 0;
}
