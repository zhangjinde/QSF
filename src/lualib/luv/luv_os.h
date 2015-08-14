// Copyright (C) 2014-2015 chenqiang@chaoyuehudong.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#include "luv.h"
#include <string.h>
#include <lua.h>
#include <lauxlib.h>
#ifndef _WIN32
#include <unistd.h>
#endif

#define MAX_PATH_LENGTH     512
#define MAX_TITLE_LENGTH    256

static int luv_version(lua_State* L) 
{
    lua_pushinteger(L, uv_version());
    return 1;
}

static int luv_version_string(lua_State* L) 
{
    lua_pushstring(L, uv_version_string());
    return 1;
}

static int luv_get_process_title(lua_State* L) 
{
    char title[MAX_TITLE_LENGTH];
    int r = uv_get_process_title(title, MAX_TITLE_LENGTH);
    if (r < 0)
    {
        return luv_error(L, r);
    }
    lua_pushstring(L, title);
    return 1;
}

static int luv_set_process_title(lua_State* L) 
{
    const char* title = luaL_checkstring(L, 1);
    int r = uv_set_process_title(title);
    if (r < 0)
    {
        return luv_error(L, r);
    }
    lua_pushinteger(L, r);
    return 1;
}

static int luv_resident_set_memory(lua_State* L) 
{
    size_t rss;
    int r = uv_resident_set_memory(&rss);
    if (r < 0)
    {
        return luv_error(L, r);
    }
    lua_pushinteger(L, rss);
    return 1;
}

static int luv_uptime(lua_State* L) 
{
    double uptime;
    int r = uv_uptime(&uptime);
    if (r < 0)
    {
        return luv_error(L, r);
    }
    lua_pushnumber(L, uptime);
    return 1;
}

static void luv_push_timeval_table(lua_State* L, const uv_timeval_t* t) 
{
    lua_createtable(L, 0, 2);
    lua_pushinteger(L, t->tv_sec);
    lua_setfield(L, -2, "sec");
    lua_pushinteger(L, t->tv_usec);
    lua_setfield(L, -2, "usec");
}

static int luv_getrusage(lua_State* L)
{
    uv_rusage_t rusage;
    int r = uv_getrusage(&rusage);
    if (r < 0)
    {
        return luv_error(L, r);
    }
    lua_createtable(L, 0, 16);
    // user CPU time used
    luv_push_timeval_table(L, &rusage.ru_utime);
    lua_setfield(L, -2, "utime");
    // system CPU time used
    luv_push_timeval_table(L, &rusage.ru_stime);
    lua_setfield(L, -2, "stime");
    // maximum resident set size
    lua_pushinteger(L, rusage.ru_maxrss);
    lua_setfield(L, -2, "maxrss");
    // integral shared memory size
    lua_pushinteger(L, rusage.ru_ixrss);
    lua_setfield(L, -2, "ixrss");
    // integral unshared data size
    lua_pushinteger(L, rusage.ru_idrss);
    lua_setfield(L, -2, "idrss");
    // integral unshared stack size
    lua_pushinteger(L, rusage.ru_isrss);
    lua_setfield(L, -2, "isrss");
    // page reclaims (soft page faults)
    lua_pushinteger(L, rusage.ru_minflt);
    lua_setfield(L, -2, "minflt");
    // page faults (hard page faults)
    lua_pushinteger(L, rusage.ru_majflt);
    lua_setfield(L, -2, "majflt");
    // swaps
    lua_pushinteger(L, rusage.ru_nswap);
    lua_setfield(L, -2, "nswap");
    // block input operations
    lua_pushinteger(L, rusage.ru_inblock);
    lua_setfield(L, -2, "inblock");
    // block output operations
    lua_pushinteger(L, rusage.ru_oublock);
    lua_setfield(L, -2, "oublock");
    // IPC messages sent
    lua_pushinteger(L, rusage.ru_msgsnd);
    lua_setfield(L, -2, "msgsnd");
    // IPC messages received
    lua_pushinteger(L, rusage.ru_msgrcv);
    lua_setfield(L, -2, "msgrcv");
    // signals received
    lua_pushinteger(L, rusage.ru_nsignals);
    lua_setfield(L, -2, "nsignals");
    // voluntary context switches
    lua_pushinteger(L, rusage.ru_nvcsw);
    lua_setfield(L, -2, "nvcsw");
    // involuntary context switches
    lua_pushinteger(L, rusage.ru_nivcsw);
    lua_setfield(L, -2, "nivcsw");
    return 1;
}

static int luv_cpu_info(lua_State* L) 
{
    uv_cpu_info_t* cpu_infos;
    int count = 0;
    int r = uv_cpu_info(&cpu_infos, &count);
    if (r < 0)
    {
        return luv_error(L, r);
    }
    lua_newtable(L);
    for (int i = 0; i < count; i++) 
    {
        lua_newtable(L);
        lua_pushstring(L, cpu_infos[i].model);
        lua_setfield(L, -2, "model");
        lua_pushinteger(L, cpu_infos[i].speed);
        lua_setfield(L, -2, "speed");
        lua_newtable(L);
        lua_pushinteger(L, cpu_infos[i].cpu_times.user);
        lua_setfield(L, -2, "user");
        lua_pushinteger(L, cpu_infos[i].cpu_times.nice);
        lua_setfield(L, -2, "nice");
        lua_pushinteger(L, cpu_infos[i].cpu_times.sys);
        lua_setfield(L, -2, "sys");
        lua_pushinteger(L, cpu_infos[i].cpu_times.idle);
        lua_setfield(L, -2, "idle");
        lua_pushinteger(L, cpu_infos[i].cpu_times.irq);
        lua_setfield(L, -2, "irq");
        lua_setfield(L, -2, "times");
        lua_rawseti(L, -2, i + 1);
    }
    uv_free_cpu_info(cpu_infos, count);
    return 1;
}

static int luv_interface_addresses(lua_State* L) 
{
    uv_interface_address_t* interfaces;
    char ip[INET6_ADDRSTRLEN];
    int count = 0;
    uv_interface_addresses(&interfaces, &count);
    lua_newtable(L);
    for (int i = 0; i < count; i++) 
    {
        lua_getfield(L, -1, interfaces[i].name);
        if (!lua_istable(L, -1)) 
        {
            lua_pop(L, 1);
            lua_newtable(L);
            lua_pushvalue(L, -1);
            lua_setfield(L, -3, interfaces[i].name);
        }
        lua_newtable(L);
        lua_pushboolean(L, interfaces[i].is_internal);
        lua_setfield(L, -2, "internal");
        if (interfaces[i].address.address4.sin_family == AF_INET) 
        {
            uv_ip4_name(&interfaces[i].address.address4, ip, sizeof(ip));
        }
        else if (interfaces[i].address.address4.sin_family == AF_INET6)
        {
            uv_ip6_name(&interfaces[i].address.address6, ip, sizeof(ip));
        }
        else 
        {
            strncpy(ip, "<unknown sa family>", INET6_ADDRSTRLEN);
        }
        lua_pushstring(L, ip);
        lua_setfield(L, -2, "ip");
        lua_pushstring(L, luv_af_num_to_string(interfaces[i].address.address4.sin_family));
        lua_setfield(L, -2, "family");
        lua_rawseti(L, -2, lua_rawlen(L, -2) + 1);
        lua_pop(L, 1);
    }
    uv_free_interface_addresses(interfaces, count);
    return 1;
}

static int luv_loadavg(lua_State* L) 
{
    double avg[3];
    uv_loadavg(avg);
    lua_pushnumber(L, avg[0]);
    lua_pushnumber(L, avg[1]);
    lua_pushnumber(L, avg[2]);
    return 3;
}

static int luv_exepath(lua_State* L) 
{
    size_t size = 2 * MAX_PATH_LENGTH;
    char exe_path[2 * MAX_PATH_LENGTH];
    int r = uv_exepath(exe_path, &size);
    if (r < 0)
    {
        return luv_error(L, r);
    }
    lua_pushlstring(L, exe_path, size);
    return 1;
}

static int luv_cwd(lua_State* L) 
{
    size_t size = 2 * MAX_PATH_LENGTH;
    char path[2 * MAX_PATH_LENGTH];
    int r = uv_cwd(path, &size);
    if (r < 0)
    {
        return luv_error(L, r);
    }
    lua_pushlstring(L, path, size);
    return 1;
}

static int luv_chdir(lua_State* L) 
{
    int r = uv_chdir(luaL_checkstring(L, 1));
    if (r < 0)
    {
        return luv_error(L, r);
    }
    lua_pushinteger(L, r);
    return 1;
}

static int luv_os_homedir(lua_State* L) 
{
    size_t size = 2 * MAX_PATH_LENGTH;
    char homedir[2 * MAX_PATH_LENGTH];
    int r = uv_os_homedir(homedir, &size);
    if (r < 0)
    {
        return luv_error(L, r);
    }
    lua_pushlstring(L, homedir, size);
    return 1;
}

static int luv_get_total_memory(lua_State* L) 
{
    lua_pushinteger(L, uv_get_total_memory());
    return 1;
}

static int luv_hrtime(lua_State* L) 
{
    lua_pushinteger(L, uv_hrtime());
    return 1;
}

static int luv_getpid(lua_State* L)
{
    int pid = getpid();
    lua_pushinteger(L, pid);
    return 1;
}

#ifndef _WIN32
static int luv_getuid(lua_State* L)
{
    int uid = getuid();
    lua_pushinteger(L, uid);
    return 1;
}

static int luv_getgid(lua_State* L)
{
    int gid = getgid();
    lua_pushinteger(L, gid);
    return 1;
}

static int luv_setuid(lua_State* L)
{
    int uid = luaL_checkinteger(L, 1);
    int r = setuid(uid);
    if (r == -1)
    {
        luaL_error(L, "Error setting UID");
    }
    return 0;
}

static int luv_setgid(lua_State* L)
{
    int gid = luaL_checkinteger(L, 1);
    int r = setgid(gid);
    if (r == -1)
    {
        luaL_error(L, "Error setting GID");
    }
    return 0;
}
#endif
