#include "core/SharedLibrary.h"
#include <gtest/gtest.h>
#include <iostream>

using namespace std;

#ifdef _WIN32
const char* libname = "lua5.2.dll";
#else
const char* libname = "liblua5.2.so";
#endif

const char* symbol_new = "luaL_newstate";
const char* symbol_close = "lua_close";
static void* (*fn_newstate)();
static void (*fn_close)(void*);


TEST(SharedLibrary, Construct)
{
    auto fn = [&]()
    {
        SharedLibrary lib(libname);
    };
    EXPECT_NO_THROW( fn() );
}

TEST(SharedLibrary, unload)
{
    SharedLibrary lib(libname);
    EXPECT_NO_THROW(lib.unload());
}

TEST(SharedLibrary, getSymbol)
{
    SharedLibrary lib(libname);
    EXPECT_FALSE(lib.hasSymbol("non_exist_symbol"));

    fn_newstate = (decltype(fn_newstate))lib.getSymbol(symbol_new);
    fn_close = (decltype(fn_close))lib.getSymbol(symbol_close);
    EXPECT_NE(fn_newstate, nullptr);
    EXPECT_NE(fn_newstate, nullptr);
    auto fn = [&]()
    {
        void* state = fn_newstate();
        fn_close(state);
    };
    EXPECT_NO_THROW(fn());

    lib.unload();
    //fn_newstate(); //SIGSEGV
}
