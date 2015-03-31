// Copyright (C) 2014-2015 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#include <string>
#include <vector>
#include <lua.hpp>
#include "service/Context.h"
#include "qsf.h"


// send message to a named service
static int mq_send(lua_State* L)
{
    Context* self = (Context*)lua_touserdata(L, lua_upvalueindex(1));
    assert(self);
    size_t name_size = 0;
    size_t data_size = 0;
    const char* name = luaL_checklstring(L, 1, &name_size);
    const char* data = luaL_checklstring(L, 2, &data_size);
    assert(data && data_size && name && name_size && name_size <= MAX_NAME_SIZE);
    self->Send(StringPiece(name, name_size), StringPiece(data, data_size));
    return 0;
}

// recv new message from another service
static int mq_recv(lua_State* L)
{
    Context* self = (Context*)lua_touserdata(L, lua_upvalueindex(1));
    assert(self);
    bool dontwait = false;
    const char* option = lua_tostring(L, -1);
    if (option)
    {
        dontwait = (strcmp(option, "nowait") == 0);
    }
    int r = 0;
    self->Recv([&](StringPiece name, StringPiece data)
    {
        if (data.size() > 0)
        {
            lua_pushlstring(L, name.data(), name.size());
            lua_pushlstring(L, data.data(), data.size());
            r = 2;
        }
    }, dontwait);

    return r;
}

static int mq_name(lua_State* L)
{
    Context* self = (Context*)lua_touserdata(L, lua_upvalueindex(1));
    assert(self);
    const auto& name = self->Name();
    lua_pushlstring(L, name.c_str(), name.length());
    return 1;
}

// launch a new service
static int mq_launch(lua_State* L)
{
    Context* self = (Context*)lua_touserdata(L, lua_upvalueindex(1));
    assert(self);
    const auto& name = self->Name();
    std::string ident = luaL_checkstring(L, 1);
    std::string args = luaL_checkstring(L, 2);
    args += " ";
    args += name;
    int top = lua_gettop(L);
    for (int i = 3; i <= top; i++)
    {
        const char* value = luaL_checkstring(L, i);
        args.append(" ");
        args.append(value);
    }
    bool r = qsf::CreateService("LuaService", ident, args);
    lua_pushboolean(L, r);
    return 1;
}

// stop all services
static int mq_shutdown(lua_State* L)
{
    qsf::Exit();
    return 0;
}

extern "C" 
int luaopen_mq(lua_State* L)
{
    static const luaL_Reg lib[] = 
    {
        { "send", mq_send },
        { "recv", mq_recv },
        { "name", mq_name },
        { "launch", mq_launch },
        { "shutdown", mq_shutdown },
        {NULL, NULL},
    };

    luaL_newlibtable(L, lib);
    lua_getfield(L, LUA_REGISTRYINDEX, "mq_ctx");
    Context* self = (Context*)lua_touserdata(L, -1);
    luaL_argcheck(L, self != nullptr, 1, "invalid context pointer");
    luaL_setfuncs(L, lib, 1);
    return 1;
}
