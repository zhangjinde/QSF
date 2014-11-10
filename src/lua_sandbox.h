// Copyright (C) 2014 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#pragma once

#include "service.h"

struct lua_State;

// lua virutal machine sand box
class LuaSandBox : public Service
{
public:
    explicit LuaSandBox(Context& ctx);
    virtual ~LuaSandBox();

    // the first argument is the main chunk file name
    virtual int run(const std::vector<std::string>& args);

private:
    void initialize();
    void loadLibPath();

private:
    lua_State*  L = nullptr;
};
