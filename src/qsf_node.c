// Copyright (C) 2014-2015 chenqiang@chaoyuehudong.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#include "qsf_node.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <zmq.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include "qsf.h"

// max dealer identity size
#define MAX_ID_LENGTH       32
#define MAX_ARG_LENGTH      256

#define qsf_zmq_assert(cond) \
    qsf_assert((cond), "zmq error: %d, %s", zmq_errno(), zmq_strerror(zmq_errno()))

// a node represent a OS thread running lua code
struct qsf_node_s
{
    qsf_node_t*         next;         // linked list
    struct lua_State*   L;            // lua VM
    uv_thread_t         thread;       // service thread
    uv_loop_t           loop;         // uv loop

    void*       dealer;               // zmq dealer
    char        name[MAX_ID_LENGTH];  // service name
    char        path[MAX_PATH];       // lua file path
    char        args[MAX_ARG_LENGTH]; // passed arguments

    uint32_t    tag;                  // used to check whether the object is good.
};

struct qsf_node_context_s
{
    int             count;      // number of services
    qsf_node_t*     list;       // single list container
    uv_mutex_t      mutex;      // service mutex
};

// global node context
static struct qsf_node_context_s node_ctx;

// forward declaration
extern void init_preload_libs(lua_State* L);

static qsf_node_t* find_from_node_list(const char* name)
{
    assert(name);
    qsf_node_t* phead = node_ctx.list;
    while (phead)
    {
        if (strcmp(phead->name, name) == 0)
        {
            return phead;
        }
        else
        {
            phead = phead->next;
        }
    }
    return NULL;
}

static int remove_from_node_list(qsf_node_t* s)
{
    assert(s);
    qsf_node_t** pph = &node_ctx.list;
    while (*pph)
    {
        if (strcmp((*pph)->name, s->name) == 0)
        {
            qsf_node_t* p = *pph;
            *pph = p->next;
            qsf_free(p);
            node_ctx.count--;
            break;
        }
        else
        {
            pph = &(*pph)->next;
        }
    }
    return 0;
}

static qsf_node_t* create_from_node_list(const char* name, const char* path, const char* args)
{
    assert(name && path && args);
    qsf_node_t* s = qsf_malloc(sizeof(qsf_node_t));
    memset(s, 0, sizeof(*s));
    strncpy(s->name, name, sizeof(s->name));
    strncpy(s->path, path, sizeof(s->path));
    strncpy(s->args, args, sizeof(s->args));

    qsf_node_t** pph = &node_ctx.list;
    while (*pph)
    {
        pph = &(*pph)->next;
    }
    *pph = s;
    node_ctx.count++;
    return s;
}

// load Lua path and Lua cpath
static void load_node_path(lua_State* L)
{
    char chunk[512];
    const char* path = qsf_getenv("lua_path");
    if (strlen(path) > 0)
    {
        int r = snprintf(chunk, sizeof(chunk), "package.path = package.path .. ';' .. '%s'", path);
        qsf_assert(r > 0, "path too long");
        r = luaL_dostring(L, chunk);
        qsf_assert(r == LUA_OK, "%s", lua_tostring(L, -1));
    }
    const char* cpath = qsf_getenv("lua_cpath");
    if (strlen(cpath) > 0)
    {
        int r = snprintf(chunk, sizeof(chunk), "package.cpath = package.cpath .. ';' .. '%s'", cpath);
        qsf_assert(r > 0, "cpath too long");
        r = luaL_dostring(L, chunk);
        qsf_assert(r == LUA_OK, "%s", lua_tostring(L, -1));
    }
}

static int init_node(qsf_node_t* s)
{
    uv_loop_init(&s->loop);
    lua_State* L = luaL_newstate();
    if (L == NULL)
    {
        return 1;
    }
    lua_gc(L, LUA_GCSTOP, 0);  // stop collector during initialization
    luaL_openlibs(L);
    init_preload_libs(L);
    load_node_path(L);
    lua_pushlightuserdata(L, s); // thus pointer `s` cannot be moved before lua_close()
    lua_setfield(L, LUA_REGISTRYINDEX, "qsf_ctx");
    lua_gc(L, LUA_GCRESTART, 0);
    s->loop.data = L;
    s->L = L;
    s->dealer = qsf_create_dealer(s->name);
    s->tag = QSF_NODE_TAG_VALUE_GOOD;
    return 0;
}

static void cleanup_node(qsf_node_t* s)
{
    if (s->dealer)
    {
        zmq_close(s->dealer);
    }
    if (s->L)
    {
        lua_close(s->L);
    }
    if (uv_loop_alive(&s->loop))
    {
        uv_stop(&s->loop);
    }
    //uv_loop_close(&s->loop);
    s->tag = QSF_NODE_TAG_VALUE_BAD;
    qsf_log("service [%s] exit.\n", s->name);
    uv_mutex_lock(&node_ctx.mutex);
    remove_from_node_list(s);
    uv_mutex_unlock(&node_ctx.mutex);
}

static void node_thread_callback(void* args)
{
    assert(args != NULL);
    qsf_node_t* s = (qsf_node_t*)args;
    if (init_node(s) == 0)
    {
        int r = luaL_loadfile(s->L, s->path);
        if (r == LUA_OK)
        {
            lua_pushstring(s->L, s->args);
            qsf_trace_pcall(s->L, 1);
        }
        else
        {
            qsf_log("%s: %s\n", s->name, lua_tostring(s->L, -1));
        }
    }
    cleanup_node(s);
}

int qsf_create_node(const char* name, const char* path, const char* args)
{
    assert(name && path && args);
    size_t len = strlen(name);
    if (len >= MAX_ID_LENGTH)
    {
        return 1; // invalid name size
    }
    if (strcmp(name, "sys") == 0)
    {
        return 2; // reserved name
    }
    
    uv_mutex_lock(&node_ctx.mutex);
    qsf_node_t* s = find_from_node_list(name);
    if (s != NULL)
    {
        uv_mutex_unlock(&node_ctx.mutex);
        return 3; // service already exist
    }
    s = create_from_node_list(name, path, args);
    int r = uv_thread_create(&s->thread, node_thread_callback, s);
	if (r < 0)
	{
		remove_from_node_list(s);
	}
    uv_mutex_unlock(&node_ctx.mutex);
    return r;
}

int qsf_node_check_tag(qsf_node_t* s)
{
    return (s && s->tag == QSF_NODE_TAG_VALUE_GOOD);
}

void qsf_node_send(qsf_node_t* s,
                   const char* name, int len, 
                   const char* data, int size)
{
    assert(s && name && len && data && size);

    int r = zmq_send(s->dealer, name, len, ZMQ_SNDMORE);
    qsf_assert(r == len, "send zmq dealer identity failed.");
    r = zmq_send(s->dealer, data, size, 0);
    qsf_assert(r == size, "send dealer message failed.");
}

int qsf_node_recv(struct qsf_node_s* s, 
                  msg_recv_handler func, 
                  int nowait, void* ud)
{
    assert(s && func);

    zmq_msg_t from;
    zmq_msg_t msg;

    qsf_zmq_assert(zmq_msg_init(&from) == 0);

    int flag = (nowait != 0 ? ZMQ_DONTWAIT : 0);
    int r = zmq_msg_recv(&from, s->dealer, flag);
    if (r > 0)
    {
        const char* name = zmq_msg_data(&from);
        size_t len = zmq_msg_size(&from);
        qsf_assert(len < MAX_ID_LENGTH, "invalid zmq peer name: %s", name);

        qsf_zmq_assert(zmq_msg_init(&msg) == 0);
        r = zmq_msg_recv(&msg, s->dealer, flag);
        if (LIKELY(r > 0))
        {
            const char* data = zmq_msg_data(&msg);
            size_t size = zmq_msg_size(&msg);
            r = func(ud, name, (int)len, data, (int)size);
        }
        qsf_zmq_assert(zmq_msg_close(&from) == 0);
        qsf_zmq_assert(zmq_msg_close(&msg) == 0);
        return (r > 0 ? r : 0);
    }
    
    qsf_zmq_assert(zmq_msg_close(&from) == 0);
    return 0;
}

uv_loop_t* qsf_node_loop(qsf_node_t* s)
{
    return &s->loop;
}

const char* qsf_node_name(qsf_node_t* s)
{
    assert(s);
    return s->name;
}

int qsf_node_run(qsf_node_t* s)
{
    return uv_run(&s->loop, UV_RUN_DEFAULT);
}

static int qsf_traceback(lua_State* L)
{
    if (!lua_isstring(L, 1))   // Non-string error object? Try metamethod.
    {
        if (lua_isnoneornil(L, 1) ||
            !luaL_callmeta(L, 1, "__tostring") ||
            !lua_isstring(L, -1))
            return 1;  // Return non-string error object.
        lua_remove(L, 1);  // Replace object by result of __tostring metamethod.
    }
    luaL_traceback(L, L, lua_tostring(L, 1), 1);
    return 1;
}

int qsf_trace_pcall(struct lua_State* L, int narg)
{
    int base = lua_gettop(L) - narg;  // function index
    lua_pushcfunction(L, qsf_traceback);  // push traceback function
    lua_insert(L, base);  // put it under chunk and args
    int r = lua_pcall(L, narg, 0, base);
    lua_remove(L, base);  // remove traceback function
    if (r != 0)
    {
        fprintf(stderr, "%s\n", lua_tostring(L, -1));
    }
    return r;
}

int qsf_node_init()
{
    int r = uv_mutex_init(&node_ctx.mutex);
    if (r < 0)
    {
        qsf_log("service: uv_mutex_init() failed.\n");
        return r;
    }

    node_ctx.count = 0;
    node_ctx.list = NULL;
    return 0;
}

void qsf_node_exit()
{
    qsf_node_t* ctx = node_ctx.list;
    while (ctx)
    {
        qsf_node_t* p = ctx;
        ctx = ctx->next;
        cleanup_node(p);
    }
    node_ctx.count = 0;
    uv_mutex_destroy(&node_ctx.mutex);
}