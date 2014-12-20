// Copyright (C) 2014 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#include "service.h"
#include "lua_service.h"
#include "shared_service.h"


ServicePtr createService(const std::string& type, Context& ctx)
{
    assert(!type.empty());
    if (type == "LuaService")
    {
        return ServicePtr(new LuaService(ctx));
    }
    else if (type == "SharedService")
    {
        return ServicePtr(new SharedService(ctx));
    }
    return ServicePtr();
}

