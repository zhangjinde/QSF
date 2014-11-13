// Copyright (C) 2014 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#pragma once

#include <string>
#include <mutex>
#include <memory>

struct lua_State;

// Global environment
class Env
{
public:
    static bool initialize(const char* file);
    static void release();
    
    // getter and setter are thread-safe
    static bool set(const char* key, const char* value);
    static std::string get(const char* key);
    static int64_t getInt(const char* key);

private:
    static bool load(lua_State* L);

private:
    static lua_State*   L_;
    static std::mutex   mutex_;
};
