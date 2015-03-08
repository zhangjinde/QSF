// Copyright (C) 2014 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#include "SharedLibrary.h"
#include "Platform.h"
#include "Logging.h"
#include <stdexcept>

SharedLibrary::SharedLibrary(const std::string& path)
    : path_(path)
{
    int r = uv_dlopen(path.c_str(), &lib_);
    if (r != 0)
    {
        throw std::runtime_error(uv_dlerror(&lib_));
    }
}

SharedLibrary::~SharedLibrary()
{
    UnLoad();
}

void SharedLibrary::UnLoad()
{
    uv_dlclose(&lib_);
}

void* SharedLibrary::GetSymbol(const std::string& name)
{
    void* symbol = nullptr;
    uv_dlsym(&lib_, name.c_str(), &symbol);
    return symbol;
}

bool SharedLibrary::HasSymbol(const std::string& name)
{
    return (GetSymbol(name) != nullptr);
}
