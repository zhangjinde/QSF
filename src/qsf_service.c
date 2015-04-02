// Copyright (C) 2014-2015 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#include "qsf_service.h"
#include <assert.h>
#include <zmq.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include "qsf.h"
#include "qsf_env.h"
#include "qsf_malloc.h"



typedef struct qsf_service_s
{
    RB_ENTRY(qsf_service_s) tree_entry;
    uv_thread_t thread;
    void* dealer;
    char name[MAX_ID_LENGTH];
    char path[MAX_PATH];
    char args[MAX_ARG_LENGTH];
    struct lua_State* L;
}qsf_service_t;

static int service_compare(qsf_service_t* a, qsf_service_t* b)
{
    assert(a && b);
    return strcmp(a->name, b->name);
}

RB_HEAD(qsf_service_tree_s, qsf_service_s);
RB_GENERATE_STATIC(qsf_service_tree_s, qsf_service_s, tree_entry, service_compare);

typedef struct qsf_service_context_s
{
    struct qsf_service_tree_s service_map;
    uv_mutex_t  mutex;
}qsf_service_context_t;


static qsf_service_context_t    service_context;

static void service_thread_callback(void* args);

int qsf_create_service(const char* name, const char* path, const char* args)
{
    assert(name && path && args);
    size_t len = strlen(name);
    if (len >= MAX_ID_LENGTH)
    {
        return 1;
    }
    if (strcmp(name, "sys") == 0) // "sys" is reserved name
    {
        return 2;
    }
    qsf_service_t* s = qsf_malloc(sizeof(qsf_service_t));
    memset(s, 0, sizeof(qsf_service_t));
    strncpy(s->name, name, len);
    strncpy(s->path, path, strlen(path));
    strncpy(s->args, args, strlen(args));

    uv_mutex_lock(&service_context.mutex);
    qsf_service_t* old = RB_FIND(qsf_service_tree_s, &service_context.service_map, s);
    if (old)
    {
        qsf_free(s);
        return 3;
    }
    uv_thread_create(&s->thread, service_thread_callback, s);
    old = RB_INSERT(qsf_service_tree_s, &service_context.service_map, s);
    assert(old == NULL);
    uv_mutex_unlock(&service_context.mutex);
    return 0;
}

static int load_service_path(lua_State* L)
{
    char chunk[512];
    const char* path = qsf_getenv("lua_path");
    if (strlen(path) > 0)
    {
        snprintf(chunk, 1024, "package.path = package.path .. ';' .. '%s'", path);
        int r = luaL_dostring(L, chunk);
        if (r != LUA_OK)
        {
            fprintf(stderr, "%s\n", lua_tostring(L, -1));
            return r;
        }
    }
    const char* cpath = qsf_getenv("lua_cpath");
    if (strlen(cpath) > 0)
    {
        snprintf(chunk, 1024, "package.cpath = package.cpath .. ';' .. '%s'", cpath);
        int r = luaL_dostring(L, chunk);
        if (r != LUA_OK)
        {
            fprintf(stderr, "%s\n", lua_tostring(L, -1));
            return r;
        }
    }
    return 0;
}

static int init_service(qsf_service_t* s)
{
    lua_State* L = luaL_newstate();
    lua_gc(L, LUA_GCSTOP, 0);  /* stop collector during initialization */
    luaL_openlibs(L);
    int r = load_service_path(L);
    if (r != 0)
    {
        lua_close(L);
        return r;
    }
    lua_pushlightuserdata(L, s->dealer);
    lua_setfield(L, LUA_REGISTRYINDEX, "mq_ctx");
    lua_gc(L, LUA_GCRESTART, 0);
    s->L = L;
    return 0;
}

static void cleanup_service(qsf_service_t* s)
{
    fprintf(stdout, "service [%s] exit.\n", s->name);
    if (s->dealer)
    {
        zmq_close(s->dealer);
    }
    if (s->L)
    {
        lua_close(s->L);
    }
    qsf_free(s);
}

static int traceback(lua_State *L)
{
    if (!lua_isstring(L, 1))  /* Non-string error object? Try metamethod. */
    {
        if (lua_isnoneornil(L, 1) ||
            !luaL_callmeta(L, 1, "__tostring") ||
            !lua_isstring(L, -1))
            return 1;  /* Return non-string error object. */
        lua_remove(L, 1);  /* Replace object by result of __tostring metamethod. */
    }
    luaL_traceback(L, L, lua_tostring(L, 1), 1);
    return 1;
}

static void run_service(qsf_service_t* s)
{
    lua_State* L = s->L;
    assert(L);
    int r = luaL_loadfile(L, s->path);
    if (r != LUA_OK)
    {
        fprintf(stderr, "%s: %s\n", s->name, lua_tostring(L, -1));
        return ;
    }
    lua_pushstring(L, s->args);
    int index = lua_gettop(L) - 1;
    lua_pushcfunction(L, traceback);  // push traceback function
    lua_insert(L, index);
    r = lua_pcall(L, 1, LUA_MULTRET, index);
    lua_remove(L, index); // remove traceback function
    if (r != LUA_OK)
    {
        fprintf(stderr, "%s: %s\n", s->name, lua_tostring(L, -1));
        return ;
    }
}

void service_thread_callback(void* args)
{
    qsf_service_t* s = (qsf_service_t*)args;
    s->dealer = qsf_create_dealer(s->name);
    if (s->dealer && init_service(s) == 0)
    {
        run_service(s);
    }    
    uv_mutex_lock(&service_context.mutex);
    RB_REMOVE(qsf_service_tree_s, &service_context.service_map, s);
    cleanup_service(s);
    uv_mutex_unlock(&service_context.mutex);
}

int qsf_service_init()
{
    int r = uv_mutex_init(&service_context.mutex);
    if (r != 0)
    {
        fprintf(stderr, "init service mutex failed.\n");
        return r;
    }
    return 0;
}

void qsf_service_exit()
{
    uv_mutex_destroy(&service_context.mutex);
}
