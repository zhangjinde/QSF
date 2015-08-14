// Copyright (C) 2014-2015 chenqiang@chaoyuehudong.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#include <assert.h>
#include <uv.h>
#include <lua.h>
#include <lauxlib.h>
#include "qsf.h"
#include "luv.h"
#include "luv_os.h"
#include "luv_timer.h"

static uv_loop_t* luv_loop(lua_State* L)
{
    qsf_node_t* self = lua_touserdata(L, lua_upvalueindex(1));
    if (!qsf_node_check_tag(self))
    {
        luaL_error(L, "invalid node object");
    }
    return qsf_node_loop(self);
}

static int luv_error(lua_State* L, int r)
{
    luaL_error(L, "%s: %s", uv_err_name(r), uv_strerror(r));
    return 0;
}

static const char* luv_af_num_to_string(int num)
{
    switch (num) 
    {
#ifdef AF_UNIX
    case AF_UNIX: return "unix";
#endif
#ifdef AF_INET
    case AF_INET: return "inet";
#endif
#ifdef AF_INET6
    case AF_INET6: return "inet6";
#endif
#ifdef AF_IPX
    case AF_IPX: return "ipx";
#endif
#ifdef AF_NETLINK
    case AF_NETLINK: return "netlink";
#endif
#ifdef AF_X25
    case AF_X25: return "x25";
#endif
#ifdef AF_AX25
    case AF_AX25: return "ax25";
#endif
#ifdef AF_ATMPVC
    case AF_ATMPVC: return "atmpvc";
#endif
#ifdef AF_APPLETALK
    case AF_APPLETALK: return "appletalk";
#endif
#ifdef AF_PACKET
    case AF_PACKET: return "packet";
#endif
    }
    return NULL;
}

static const luaL_Reg libs[] =
{
    { "chdir", luv_chdir },
    { "osHomedir", luv_os_homedir },
    { "cpuInfo", luv_cpu_info },
    { "cwd", luv_cwd },
    { "exepath", luv_exepath },
    { "getProcessTitle", luv_get_process_title },
    { "getTotalMemory", luv_get_total_memory },
    { "getpid", luv_getpid },
#ifndef _WIN32
    { "getuid", luv_getuid },
    { "setuid", luv_setuid },
    { "getgid", luv_getgid },
    { "setgid", luv_setgid },
#endif
    { "getrusage", luv_getrusage },
    { "hrtime", luv_hrtime },
    { "interfaceAddresses", luv_interface_addresses },
    { "loadavg", luv_loadavg },
    { "residentSetMemory", luv_resident_set_memory },
    { "setProcessTitle", luv_set_process_title },
    { "uptime", luv_uptime },
    { "version", luv_version },
    { "versionString", luv_version_string },

    // timer
    { "createTimer", luv_new_timer },

    { NULL, NULL }
};

LUALIB_API int luaopen_luv(lua_State* L)
{
    luaL_newlibtable(L, libs);
    lua_getfield(L, LUA_REGISTRYINDEX, "qsf_ctx");
    qsf_node_t* self = lua_touserdata(L, -1);
    luaL_argcheck(L, self != NULL, 1, "invalid context pointer");
    luaL_setfuncs(L, libs, 1);
    create_timer_meta(L);
    return 1;
}