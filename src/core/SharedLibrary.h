// Copyright (C) 2014 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#pragma once

#include <string>
#include <uv.h>


// Dynamically loads shared libraries at run-time.
class SharedLibrary
{
public:
    explicit SharedLibrary(const std::string& path);
    virtual ~SharedLibrary();

    SharedLibrary(const SharedLibrary&) = delete;
    SharedLibrary& operator = (const SharedLibrary&) = delete;

    void UnLoad();
    bool HasSymbol(const std::string& name);
    void* GetSymbol(const std::string& name);

    const std::string& path() { return path_; }

private:
    std::string     path_;
    uv_lib_t        lib_;
};