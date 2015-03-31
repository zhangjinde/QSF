// Copyright (C) 2014-2015 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#pragma once

#include <cassert>
#include <memory>
#include <string>
#include "Context.h"

// A service is an execution unit scheduled to an individual OS thread
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

    // Type of this service, different services can have same type
    const std::string&  GetType() const { return type_; }
    
    // Context object of this service
    Context&  GetCtx() { return ctx_; }

    // Run this service, virtual interface
    virtual int Run(const std::string& args) = 0;

private:
    Context&        ctx_;
    std::string     type_;
};

typedef std::shared_ptr<Service>    ServicePtr;

// Create a new service and bind a context to it, `type` must be unique.
ServicePtr CreateService(const std::string& type, Context& ctx);
