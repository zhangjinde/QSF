// Copyright (C) 2014-2015 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#include "Service.h"
#include "LuaService.h"


ServicePtr CreateService(const std::string& type, Context& ctx)
{
    assert(!type.empty());
    if (type == "LuaService")
    {
        return ServicePtr(new LuaService(ctx));
    }
    assert(false && "unsupported service type");
    return ServicePtr();
}

