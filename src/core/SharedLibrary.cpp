// Copyright (C) 2014 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#include "SharedLibrary.h"
#include "Platform.h"
#include "Logging.h"
#include <errno.h>
#include <stdexcept>


#ifdef _WIN32
#include <windows.h>
inline std::string lastErrorMessage(int err)
{
    char buffer[512] = {};
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, err, 
        0, buffer, 511, nullptr);
    return buffer;
}
#else
#include <dlfcn.h>
#endif


SharedLibrary::SharedLibrary(const std::string& path, int flags)
    : path_(path), handle_(nullptr)
{
#ifdef _WIN32
    handle_ = LoadLibraryExA(path_.c_str(), nullptr, LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    if (handle_ == nullptr)
    {
        throw std::runtime_error(lastErrorMessage(GetLastError()));
    }
#else
    int realFlags = RTLD_LAZY;
    if (flags & SHLIB_LOCAL)
        realFlags |= RTLD_LOCAL;
    else
        realFlags |= RTLD_GLOBAL;
    handle_ = dlopen(path.c_str(), realFlags);
    if (handle_ == nullptr)
    {
        const char* err = dlerror();
        throw std::runtime_error((err == nullptr ? "" : err));
    }
#endif
}

SharedLibrary::~SharedLibrary()
{
    unload();
}

void SharedLibrary::unload()
{
    if (handle_)
    {
#ifdef _WIN32
        CHECK(FreeLibrary((HMODULE)handle_) != 0);
#else
        CHECK(dlclose(handle_) == 0);
#endif
        handle_ = nullptr;
    }
}

void* SharedLibrary::getSymbol(const std::string& name)
{
    if (handle_)
    {
#ifdef _WIN32
        return (void*)GetProcAddress((HMODULE)handle_, name.c_str());
#else
        return dlsym(handle_, name.c_str());
#endif
    }
    return nullptr;
}

bool SharedLibrary::hasSymbol(const std::string& name)
{
    return (getSymbol(name) != nullptr);
}
