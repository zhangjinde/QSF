// Copyright (C) 2014 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#pragma once

#include <string>
#include <mutex>
#include <memory>

struct lua_State;

// global environment
class Env
{
public:
    static bool Init(const char* file);
    static void Release();
    
    // getter and setter are thread-safe
    static bool Set(const char* key, const char* value);
    static std::string Get(const char* key);
    static int64_t GetInt(const char* key);

private:
    static bool Load(lua_State* L);

private:
    static lua_State*   L_;
    static std::unique_ptr<std::mutex> mutex_;
};
