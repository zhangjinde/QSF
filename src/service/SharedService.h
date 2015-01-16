// Copyright (C) 2014 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#pragma once

#include "Service.h"
#include "core/SharedLibrary.h"


class SharedService : public Service
{
public:
    explicit SharedService(Context& ctx);
    ~SharedService();

    virtual int run(const std::vector<std::string>& args);

private:
    void initialize(const std::string& path);

private:
    int (*on_run_)(void*, const char*) = nullptr;
    std::unique_ptr<SharedLibrary>  this_lib_;
};