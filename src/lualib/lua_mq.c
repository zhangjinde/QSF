// Copyright (C) 2014-2015 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#include <assert.h>
#include <lua.h>
#include <lauxlib.h>
#include "qsf_service.h"
#include "qsf_malloc.h"


// Send message to a named service
static int mq_send(lua_State* L)
{
    struct qsf_service_s* self = lua_touserdata(L, lua_upvalueindex(1));
    assert(self);
    size_t len = 0;
    size_t size = 0;
    const char* name = luaL_checklstring(L, 1, &len);
    const char* data = luaL_checklstring(L, 2, &size);
    qsf_service_send(self, name, len, data, size);
    return 0;
}

static int handle_recv(void* ud,
                       const char* name, size_t len,
                       const char* data, size_t size)
{
    assert(ud && name && len && data && size);
    lua_State* L = ud;
    if (len > 0 && size > 0)
    {
        lua_pushlstring(L, name, len);
        lua_pushlstring(L, data, size);
        return 2;
    }
    return 1;
}

// Recv new message from another service
static int mq_recv(lua_State* L)
{
    struct qsf_service_s* self = lua_touserdata(L, lua_upvalueindex(1));
    assert(self);
    int nowait = 0;
    const char* option = lua_tostring(L, -1);
    if (option)
    {
        nowait = (strcmp(option, "nowait") == 0);
    }
    int r = qsf_service_recv(self, handle_recv, nowait, L);
    return r;
}

static int mq_name(lua_State* L)
{
    struct qsf_service_s* self = lua_touserdata(L, lua_upvalueindex(1));
    assert(self);
    const char* name = qsf_service_name(self);
    lua_pushstring(L, name);
    return 1;
}

// Launch a new service
static int mq_launch(lua_State* L)
{
    struct qsf_service_s* self = lua_touserdata(L, lua_upvalueindex(1));
    assert(self);
    const char* ident = luaL_checkstring(L, 1);
    const char* path = luaL_checkstring(L, 2);
    size_t len;
    const char* args = luaL_optlstring(L, 3, "", &len);
    const char* name = qsf_service_name(self);
    size_t size = len + strlen(name) + 2;
    char* buf = qsf_malloc(size);
    snprintf(buf, size, "%s %s", name, args);
    int r = qsf_create_service(ident, path, args);
    lua_pushboolean(L, r == 0);
    qsf_free(buf);
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

    luaL_newlibtable(L, lib);
    lua_getfield(L, LUA_REGISTRYINDEX, "mq_ctx");
    struct qsf_service_s* self = lua_touserdata(L, -1);
    luaL_argcheck(L, self != NULL, 1, "invalid context pointer");
    luaL_setfuncs(L, lib, 1);
    return 1;
}