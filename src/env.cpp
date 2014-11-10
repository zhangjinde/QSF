// Copyright (C) 2014 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#include "env.h"
#include <cassert>
#include <lua.hpp>
#include "core/conv.h"


lua_State*  Env::L_ = nullptr;
std::mutex  Env::mutex_;

//////////////////////////////////////////////////////////////////////////

bool Env::initialize(const char* file)
{
    if (file == nullptr)
    {
        return false;
    }
    assert(L_ == nullptr);
    L_ = luaL_newstate();
    assert(L_);
    int err = luaL_dofile(L_, file);
    if (err)
    {
        fprintf(stderr, "%s\n", lua_tostring(L_, -1));
        lua_close(L_);
        L_ = nullptr;
        return false;
    }
    return true;
}

void Env::release()
{
    std::lock_guard<std::mutex> guard(mutex_);
    if (L_)
    {
        lua_close(L_);
        L_ = nullptr;
    }
}

bool Env::load(lua_State* L)
{
    lua_pushglobaltable(L);
    lua_pushnil(L);  /* first key */
    while (lua_next(L, -2) != 0) 
    {
        int keyt = lua_type(L, -2);
        if (keyt != LUA_TSTRING) 
        {
            fprintf(stderr, "invalid config table\n");
            return false;
        }
        const char* key = lua_tostring(L, -2);
        if (lua_type(L, -1) == LUA_TBOOLEAN) 
        {
            int b = lua_toboolean(L, -1);
            set(key, b ? "true" : "false");
        }
        else 
        {
            const char* value = lua_tostring(L, -1);
            if (value == NULL) 
            {
                fprintf(stderr, "invalid config table key = %s\n", key);
                return false;
            }
            set(key, value);
        }
        lua_pop(L, 1);
    }
    lua_pop(L, 1);
    return true;
}

bool Env::set(const char* key, const char* value)
{
    assert(key && value);
    std::lock_guard<std::mutex> guard(mutex_);
    lua_getglobal(L_, key);
    int t = lua_type(L_, -1);
    if (!lua_isnil(L_, -1))
    {
        lua_pop(L_, 1);
        return false;
    }
    lua_pop(L_, 1);
    lua_pushstring(L_, value);
    lua_setglobal(L_, key);
    return true;
}

std::string Env::get(const char* key)
{
    assert(key);
    std::lock_guard<std::mutex> guard(mutex_);
    lua_getglobal(L_, key);
    const char* r = lua_tostring(L_, -1);
    lua_pop(L_, -1);
    return r ? r : "";
}

int64_t Env::getInt(const char* key)
{
    assert(key);
    std::string str = get(key);
    return (!str.empty() ? to<int64_t>(str) : 0);
}
