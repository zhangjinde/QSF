// Copyright (C) 2014 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#pragma once

#include <string>
#include <uv.h>

//
// Dynamically loads shared libraries at run-time.
//

class SharedLibrary
{
public:
        /// On platforms that use dlopen(), use RTLD_GLOBAL. This is the default
        /// if no flags are given.
        ///
        /// This flag is ignored on platforms that do not use dlopen().

        /// On platforms that use dlopen(), use RTLD_LOCAL instead of RTLD_GLOBAL.
        ///
        /// Note that if this flag is specified, RTTI (including dynamic_cast and throw) will
        /// not work for types defined in the shared library with GCC and possibly other
        /// compilers as well. See http://gcc.gnu.org/faq.html#dso for more information.
        ///
        /// This flag is ignored on platforms that do not use dlopen().

    explicit SharedLibrary(const std::string& path);
    virtual ~SharedLibrary();

    SharedLibrary(const SharedLibrary&) = delete;
    SharedLibrary& operator = (const SharedLibrary&) = delete;

    void unload();
    bool hasSymbol(const std::string& name);
    void* getSymbol(const std::string& name);

    const std::string& path() { return path_; }

private:
    std::string     path_;
    uv_lib_t        lib_;
};