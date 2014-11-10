// Copyright (C) 2014 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#pragma once

#include <cassert>
#include <memory>
#include <string>
#include <vector>
#include "context.h"


class Service;

typedef std::shared_ptr<Service>    ServicePtr;

// service is an execution unit binding to an individual thread
class Service
{
public:
    Service(const std::string& type, Context& ctx) 
        : ctx_(ctx), type_(type)
    {
    }

    virtual ~Service() 
    {
    }

    Service(const Service&) = delete;
    Service& operator = (const Service&) = delete;

    const std::string&  type() const { return type_; }

    Context&  context() { return ctx_; }

    // virtual interface
    virtual int run(const std::vector<std::string>& args) = 0;

private:
    Context&        ctx_;
    std::string     type_;
};

ServicePtr createService(const std::string& name, Context& ctx);
