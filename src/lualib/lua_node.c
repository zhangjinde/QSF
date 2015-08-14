// Copyright (C) 2014-2015 chenqiang@chaoyuehudong.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#include <assert.h>
#include <string.h>
#include <lua.h>
#include <lauxlib.h>
#include "qsf.h"


// Send message to a named node
static int node_send(lua_State* L)
{
    qsf_node_t* self = lua_touserdata(L, lua_upvalueindex(1));
    if (!qsf_node_check_tag(self))
    {
        return luaL_error(L, "invalid node object");
    }
    size_t len = 0;
    size_t size = 0;
    const char* name = luaL_checklstring(L, 1, &len);
    const char* data = luaL_checklstring(L, 2, &size);
    qsf_node_send(self, name, (int)len, data, (int)size);
    return 0;
}

static int handle_recv(void* ud,
                       const char* name, int len,
                       const char* data, int size)
{
    assert(ud && name && len && data && size);
    lua_State* L = ud;
    if (len > 0 && size > 0)
    {
        lua_pushlstring(L, name, len);
        lua_pushlstring(L, data, size);
        return 2;
    }
    return 0;
}

// Recv new message from another service
static int node_recv(lua_State* L)
{
    qsf_node_t* self = lua_touserdata(L, lua_upvalueindex(1));
    if (!qsf_node_check_tag(self))
    {
        return luaL_error(L, "invalid node object");
    }
    int nowait = 0;
    const char* option = lua_tostring(L, -1);
    if (option)
    {
        nowait = (strcmp(option, "nowait") == 0);
    }
    int r = qsf_node_recv(self, handle_recv, nowait, L);
    return r;
}

static int node_name(lua_State* L)
{
    qsf_node_t* self = lua_touserdata(L, lua_upvalueindex(1));
    if (!qsf_node_check_tag(self))
    {
        return luaL_error(L, "invalid node object");
    }
    const char* name = qsf_node_name(self);
    lua_pushstring(L, name);
    return 1;
}

// Launch a new service
static int node_launch(lua_State* L)
{
    qsf_node_t* self = lua_touserdata(L, lua_upvalueindex(1));
    if (!qsf_node_check_tag(self))
    {
        return luaL_error(L, "invalid node object");
    }
    const char* ident = luaL_checkstring(L, 1);
    const char* path = luaL_checkstring(L, 2);
    const char* father = qsf_node_name(self);
    const char* args = "";
    if (lua_gettop(L) > 2)
    {
        size_t len = 0;
        args = luaL_checklstring(L, 3, &len);
    }
    
    char buffer[512];
    snprintf(buffer, sizeof(buffer), "%s %s", father, args);
    int r = qsf_create_node(ident, path, args);
    lua_pushboolean(L, r == 0);
    return 1;
}

static int node_run(lua_State* L)
{
    qsf_node_t* self = lua_touserdata(L, lua_upvalueindex(1));
    if (!qsf_node_check_tag(self))
    {
        return luaL_error(L, "invalid node object");
    }
    int r = qsf_node_run(self);
    return 0;
}

LUALIB_API int luaopen_node(lua_State* L)
{
    static const luaL_Reg lib[] = 
    {
        { "send", node_send },
        { "recv", node_recv },
        { "name", node_name },
        { "run", node_run },
        { "launch", node_launch },
        {NULL, NULL},
    };

    luaL_newlibtable(L, lib);
    lua_getfield(L, LUA_REGISTRYINDEX, "qsf_ctx");
    qsf_node_t* self = lua_touserdata(L, -1);
    luaL_argcheck(L, self != NULL, 1, "invalid context pointer");
    luaL_setfuncs(L, lib, 1);
    return 1;
}
