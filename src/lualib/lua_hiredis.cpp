// Copyright (C) 2014 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the New BSD License.
// See accompanying files LICENSE.

#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include <lua.hpp>
#include <hiredis.h>

typedef struct _luaRedisClient
{
    int pipeline;
    redisContext* context;
}luaRedisClient;

#define MAX_COMMAND_ARGS    32
#define LUA_HIREDIS_CLIENT "luaRedisClient*"
#define check_client(L)    ((luaRedisClient*)luaL_checkudata(L, 1, LUA_HIREDIS_CLIENT))


/* Only TCP protocol */
static int luahiredis_connect(lua_State* L)
{
    const char* host = luaL_checkstring(L, 1);
    int port = (int)luaL_checkinteger(L, 2);
    int sec = (int)luaL_optinteger(L, 3, -1);
    redisContext* c = NULL;
    if (sec > 0)
    {
        struct timeval timeout = { sec, 0 };
        c = redisConnectWithTimeout(host, port, timeout);
    }
    else
    {
        c = redisConnect(host, port);
    }
    if (c == NULL)
    {
        return luaL_error(L, "redisConnect() failed.");
    }
    if (c->err)
    {
        int err = c->err;
        char errstr[128];
        strncpy(errstr, c->errstr, 128);
        redisFree(c);
        return luaL_error(L, "redis connect failed: %d, %s.", err, errstr);
    }
    luaRedisClient* client = (luaRedisClient*)lua_newuserdata(L, sizeof(luaRedisClient));
    client->pipeline = 0;
    client->context = c;
    luaL_getmetatable(L, LUA_HIREDIS_CLIENT);
    lua_setmetatable(L, -2);
    return 1;
}

static int hiredis_client_gc(lua_State* L)
{
    luaRedisClient* client = (luaRedisClient*)luaL_checkudata(L, 1, LUA_HIREDIS_CLIENT);
    if (client->context != NULL)
    {
        redisFree(client->context);
        client->context = NULL;
        client->pipeline = 0;
    }
    return 0;
}

#define hiredis_client_close   hiredis_client_gc

static int hiredis_client_tostring(lua_State* L)
{
    luaRedisClient* c = check_client(L);
    luaL_argcheck(L, c, 1, "invalid Connection object");
    lua_pushfstring(L, "luaRedisClient* (%p)", c);
    return 1;
}

static int hiredis_client_settimeout(lua_State* L)
{
    luaRedisClient* c = check_client(L);
    int sec = (int)luaL_checkinteger(L, 2);
    struct timeval timeout = { sec, 0 };
    if (redisSetTimeout(c->context, timeout) != REDIS_OK)
    {
        return luaL_error(L, c->context->errstr);
    }
    return 0;
}

static int parse_reply_value(lua_State* L, redisReply* reply)
{
    assert(L && reply);
    switch (reply->type)
    {
    case REDIS_REPLY_STRING:
        {
            lua_pushlstring(L, reply->str, reply->len);
            return 1;
        }
    case REDIS_REPLY_INTEGER:
        {
            // default integer type is long long in Lua 5.3
            lua_pushinteger(L, reply->integer);
            return 1;
        }
    case REDIS_REPLY_NIL:
        {
            lua_pushnil(L);
            return 1;
        }
    case REDIS_REPLY_STATUS:
        {
            lua_pushlstring(L, reply->str, reply->len);
            return 1;
        }
    case REDIS_REPLY_ERROR:
        {
            lua_pushlstring(L, reply->str, reply->len);
            return 1;
        }
    }
    return 0;
}

static int parse_reply(lua_State* L, redisReply* reply)
{
    assert(reply);
    if (reply->type != REDIS_REPLY_ARRAY)
    {
        return parse_reply_value(L, reply);
    }
    // array result
    lua_createtable(L, reply->elements, 0);
    for (size_t i = 0; i < reply->elements; i++)
    {
        parse_reply_value(L, reply->element[i]);
        lua_rawseti(L, -2, i + 1);
    }
    return 1;
}

static int hiredis_client_command(lua_State* L)
{
    luaRedisClient*  client = check_client(L);
    redisContext* ctx = client->context;
    redisReply* reply = NULL;

    int nargs = lua_gettop(L) - 1;
    if (nargs < 2)
    {
        const char* command = luaL_checkstring(L, 2);
        reply = (redisReply*)redisCommand(ctx, command);
    }
    else
    {
        if (nargs > MAX_COMMAND_ARGS)
        {
            return luaL_error(L, "arguments error.");
        }
        const char* argv[MAX_COMMAND_ARGS] = { NULL };
        size_t argvlen[MAX_COMMAND_ARGS] = { 0 };
        for (int i = 0; i < nargs; i++)
        {
            argv[i] = luaL_checklstring(L, i + 2, &argvlen[i]);
        }
        reply = (redisReply*)redisCommandArgv(ctx, nargs, argv, argvlen);
    }
    if (reply == NULL)
    {
        return luaL_error(L, ctx->errstr);
    }
    if (reply->type == REDIS_REPLY_ERROR)
    {
        luaL_where(L, 1);
        lua_pushlstring(L, reply->str, reply->len);
        lua_concat(L, 2);
        freeReplyObject(reply);
        return lua_error(L);
    }
    int r = parse_reply(L, reply);
    freeReplyObject(reply);
    return r;
}


static int hiredis_client_append_command(lua_State* L)
{
    luaRedisClient* client = check_client(L);
    redisContext* ctx = client->context;
    int res = REDIS_ERR;
    int nargs = lua_gettop(L) - 1;
    if (nargs < 2)
    {
        const char* command = luaL_checkstring(L, 2);
        res = redisAppendCommand(ctx, command);
    }
    else
    {
        if (nargs > MAX_COMMAND_ARGS)
        {
            return luaL_error(L, "arguments error.");
        }
        const char* argv[MAX_COMMAND_ARGS] = { NULL };
        size_t argvlen[MAX_COMMAND_ARGS] = { 0 };
        for (int i = 0; i < nargs; i++)
        {
            argv[i] = luaL_checklstring(L, i + 2, &argvlen[i]);
        }
        res = redisAppendCommandArgv(ctx, nargs, argv, argvlen);
    }
    if (res != REDIS_OK)
    {
        return luaL_error(L, ctx->errstr);
    }
    client->pipeline++;
    return 0;
}

static int hiredis_client_get_reply(lua_State* L)
{
    luaRedisClient* client = check_client(L);
    redisContext* ctx = client->context;
    redisReply* reply = NULL;
    int index = 1;
    lua_newtable(L);
    while (client->pipeline-- 
           && (redisGetReply(ctx, (void**)&reply) == REDIS_OK))
    {
        parse_reply(L, reply);
        lua_rawseti(L, -2, index++);
        freeReplyObject(reply);
    }
    return 1;
}


static void make_meta(lua_State* L)
{
    static const luaL_Reg methods[] =
    {
        { "__gc", hiredis_client_gc },
        { "__tostring", hiredis_client_tostring },
        { "close", hiredis_client_close },
        { "settimeout", hiredis_client_settimeout },
        { "command", hiredis_client_command },
        { "append_command", hiredis_client_append_command },
        { "get_reply", hiredis_client_get_reply },
        { NULL, NULL },
    };
    if (luaL_newmetatable(L, LUA_HIREDIS_CLIENT))
    {
        lua_pushvalue(L, -1);
        lua_setfield(L, -2, "__index");
        luaL_setfuncs(L, methods, 0);
        lua_pop(L, 1);  /* pop new metatable */
    }
    else
    {
        luaL_error(L, "`%s` already registered.", LUA_HIREDIS_CLIENT);
    }
}

extern "C" 
int luaopen_hiredis(lua_State* L)
{
    static const luaL_Reg lib[] =
    {
        { "connect", luahiredis_connect },
        { NULL, NULL },
    };
    luaL_checkversion(L);
    make_meta(L);
    luaL_newlib(L, lib);
    return 1;
}

