// Copyright (C) 2014 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#include "service.h"
#include "lua_sandbox.h"


ServicePtr CreateService(const std::string& type, Context& ctx)
{
    assert(!type.empty());
    if (type == "luasandbox")
    {
        return std::move(ServicePtr(new LuaSandBox(ctx)));
    }
    return ServicePtr();
}

