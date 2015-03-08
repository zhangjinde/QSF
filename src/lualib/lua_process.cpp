// Copyright (C) 2014 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#include <lua.hpp>
#include <thread>
#include <chrono>
#include <uv.h>

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

static int process_hrtime(lua_State* L)
{
    uint64_t time_point = uv_hrtime();
    lua_pushinteger(L, (int64_t)time_point);
    return 1;
}

static int process_total_memory(lua_State* L)
{
    uint64_t bytes = uv_get_total_memory();
    lua_pushinteger(L, (int64_t)bytes);
    return 1;
}

static int process_os(lua_State* L)
{
#ifdef _WIN32
    lua_pushliteral(L, "windows");
#elif defined(__linux__)
    lua_pushliteral(L, "linux");
#else defined(__APPLE__)
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

static int process_cwd(lua_State* L)
{
    char path[260];
    size_t size = sizeof(path);
    int r = uv_cwd(path, &size);
    if (r < 0)
    {
        return luaL_error(L, "cwd: %s", uv_strerror(r));
    }
    lua_pushlstring(L, path, size);
    return 1;
}

static int process_exepath(lua_State* L)
{
    char path[260];
    size_t size = sizeof(path);
    int r = uv_exepath(path, &size);
    if (r < 0)
    {
        return luaL_error(L, "exepath: %s", uv_strerror(r));
    }
    lua_pushlstring(L, path, size);
    return 1;
}

static int process_chdir(lua_State* L)
{
    const char* path = luaL_checkstring(L, 1);
    int r = uv_chdir(path);
    if (r < 0)
    {
        return luaL_error(L, "chdir: %s", uv_strerror(r));
    }
    return 0;
}

static int process_mkdir(lua_State* L)
{
    const char* path = luaL_checkstring(L, 1);
#ifdef _WIN32
    int r = CreateDirectory(path, NULL);
#else
    int r = (mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0);
#endif
    if (!r)
    {
        return luaL_error(L, "unable to create directory '%s'", path);
    }
    return 0;
}

static int process_set_title(lua_State* L)
{
    size_t len = 0;
    const char* title = luaL_checklstring(L, 1, &len);
    uv_set_process_title(title);
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
        { "hrtime", process_hrtime },
        { "total_memory", process_total_memory },
        { "cwd", process_cwd },
        { "exepath", process_exepath },
        { "chdir", process_chdir },
        { "mkdir", process_mkdir },
        { "set_title", process_set_title },
        { NULL, NULL },
    };
    luaL_newlib(L, lib);
    return 1;
}
