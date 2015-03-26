// Copyright (C) 2014 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#include <string>
#include <thread>
#include <chrono>
#include <lua.hpp>

#ifndef _WIN32
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#else
#include <Windows.h>
#define getpid  GetCurrentProcessId
inline std::wstring Utf8ToWide(const std::string& strUtf8)
{
    std::wstring strWide;
    int count = MultiByteToWideChar(CP_UTF8, 0, strUtf8.data(), (int)strUtf8.length(), NULL, 0);
    if (count > 0)
    {
        strWide.resize(count);
        MultiByteToWideChar(CP_UTF8, 0, strUtf8.data(), (int)strUtf8.length(),
            const_cast<wchar_t*>(strWide.data()), (int)strWide.length());
    }
    return strWide;
}
#endif

static int process_sleep(lua_State* L)
{
    int msec = (int)luaL_checkinteger(L, 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(msec));
    return 0;
}

static int process_gettick(lua_State* L)
{
    using namespace std::chrono;
    auto duration = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
    lua_pushinteger(L, (int64_t)duration.count());
    return 1;
}

static int process_os(lua_State* L)
{
#if defined(_WIN32)
    lua_pushliteral(L, "windows");
#elif defined(__linux__)
    lua_pushliteral(L, "linux");
#elif defined(__APPLE__)
    lua_pushliteral(L, "macos");
#else
#error platform not supported
#endif
    return 1;
}

static int process_pid(lua_State* L)
{
    int pid = getpid();
    lua_pushinteger(L, pid);
    return 1;
}

static int process_set_title(lua_State* L)
{
#ifdef _WIN32
    size_t len = 0;
    const char* title = luaL_checklstring(L, 1, &len);
    auto str = Utf8ToWide(title);
    SetConsoleTitleW(str.c_str());
#endif
    return 0;
}

extern "C" 
int luaopen_process(lua_State* L)
{
    static const luaL_Reg lib[] =
    {
        { "sleep", process_sleep },
        { "gettick", process_gettick },
        { "os", process_os },
        { "pid", process_pid },
        { "set_title", process_set_title },
        { NULL, NULL },
    };
    luaL_newlib(L, lib);
    return 1;
}
