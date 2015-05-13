// Copyright (C) 2014-2015 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#include <assert.h>
#include <string.h>
#include <lua.h>
#include <lauxlib.h>
#include "qsf.h"

static qsf_service_t* to_context(lua_State* L)
{
    lua_pushlstring(L, "mq_ctx", 6);
    lua_rawget(L, LUA_REGISTRYINDEX);
    qsf_service_t* self = lua_touserdata(L, -1);
    assert(self != NULL && "invalid context pointer");
    lua_pop(L, 1);
    return self;
}

// Send message to a named service
static int mq_send(lua_State* L)
{
    qsf_service_t* self = to_context(L);
    assert(self);
    size_t len = 0;
    size_t size = 0;
    const char* name = luaL_checklstring(L, 1, &len);
    const char* data = luaL_checklstring(L, 2, &size);
    qsf_service_send(self, name, (int)len, data, (int)size);
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
static int mq_recv(lua_State* L)
{
    qsf_service_t* self = to_context(L);
    assert(self);
    int nowait = 0;
    if (lua_gettop(L) >= 1)
    {
        const char* option = lua_tostring(L, 1);
        if (option)
        {
            nowait = (strcmp(option, "nowait") == 0);
        }
    }
    int r = qsf_service_recv(self, handle_recv, nowait, L);
    return r;
}

static int mq_name(lua_State* L)
{
    qsf_service_t* self = to_context(L);
    assert(self);
    const char* name = qsf_service_name(self);
    lua_pushstring(L, name);
    return 1;
}

// Launch a new service
static int mq_launch(lua_State* L)
{
    qsf_service_t* self = to_context(L);
    assert(self);
    const char* ident = luaL_checkstring(L, 1);
    const char* path = luaL_checkstring(L, 2);
    const char* father = qsf_service_name(self);
    const char* args = "";
    if (lua_gettop(L) > 2)
    {
        size_t len = 0;
        args = luaL_checklstring(L, 3, &len);
    }
    
    char buffer[512];
    snprintf(buffer, sizeof(buffer), "%s %s", father, args);
    int r = qsf_create_service(ident, path, args);
    lua_pushboolean(L, r == 0);
    return 1;
}

LUALIB_API int luaopen_mq(lua_State* L)
{
    static const luaL_Reg lib[] = 
    {
        { "send", mq_send },
        { "recv", mq_recv },
        { "name", mq_name },
        { "launch", mq_launch },
        {NULL, NULL},
    };
    
    luaL_register(L, "mq", lib);
    return 1;
}
