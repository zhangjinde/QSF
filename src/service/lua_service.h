// Copyright (C) 2014 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#pragma once

#include "service.h"

struct lua_State;

// Lua sand box service
class LuaService : public Service
{
public:
    explicit LuaService(Context& ctx);
    virtual ~LuaService();

    // First argument is the main chunk file name
    virtual int run(const std::vector<std::string>& args);

private:
    // Pre-load modules
    void initialize();

    // Load lua module searching path
    void loadLibPath();

private:
    lua_State*  L = nullptr;
};