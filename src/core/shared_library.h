#pragma once

#include <string>

//
// Dynamically loads shared libraries at run-time.
//

class SharedLibrary
{
public:
    enum Flags
    {
        /// On platforms that use dlopen(), use RTLD_GLOBAL. This is the default
        /// if no flags are given.
        ///
        /// This flag is ignored on platforms that do not use dlopen().
        SHLIB_GLOBAL = 1,

        /// On platforms that use dlopen(), use RTLD_LOCAL instead of RTLD_GLOBAL.
        ///
        /// Note that if this flag is specified, RTTI (including dynamic_cast and throw) will
        /// not work for types defined in the shared library with GCC and possibly other
        /// compilers as well. See http://gcc.gnu.org/faq.html#dso for more information.
        ///
        /// This flag is ignored on platforms that do not use dlopen().
        SHLIB_LOCAL = 2
    };

    explicit SharedLibrary(const std::string& path, int flags = 0);
    virtual ~SharedLibrary();

    SharedLibrary(const SharedLibrary&) = delete;
    SharedLibrary& operator = (const SharedLibrary&) = delete;

    void unload();
    bool hasSymbol(const std::string& name);
    void* getSymbol(const std::string& name);

private:
    std::string     path_;
    void*           handle_;
};