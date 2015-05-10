// Copyright (C) 2014-2015 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#include "qsf_service.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <zmq.h>
#include <uv.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include "qsf.h"


// max dealer identity size
#define MAX_ID_LENGTH       16
#define MAX_ARG_LENGTH      512

#define qsf_zmq_assert(cond) \
    qsf_assert((cond), "zmq error: %d, %s", zmq_errno(), zmq_strerror(zmq_errno()))

// qsf service type definition
struct qsf_service_s
{
    qsf_service_t*    next;         // linked list
    uv_thread_t       thread;       // service thread
    struct lua_State* L;            // lua VM

    void*   dealer;                 // zmq dealer
    char    name[MAX_ID_LENGTH];    // service name
    char    path[MAX_PATH];         // lua file path
    char    args[MAX_ARG_LENGTH];   // passed arguments
};

struct qsf_service_context_s
{
    int             count;      // number of services
    qsf_service_t*  list;       // single list container
    uv_mutex_t      mutex;      // service mutex
};

typedef struct qsf_service_context_s qsf_service_context_t;


// global service context
static qsf_service_context_t    service_context;

// forward declaration
extern void initlibs(lua_State* L);

static qsf_service_t* find_from_service_list(const char* name)
{
    assert(name);
    qsf_service_t* phead = service_context.list;
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

static int remove_from_service_list(qsf_service_t* s)
{
    assert(s);
    qsf_service_t** pph = &service_context.list;
    while (*pph)
    {
        if (strcmp((*pph)->name, s->name) == 0)
        {
            qsf_service_t* p = *pph;
            *pph = p->next;
            qsf_free(p);
            service_context.count--;
            break;
        }
        else
        {
            pph = &(*pph)->next;
        }
    }
    return 0;
}

static qsf_service_t* create_from_service_list(const char* name, const char* path, const char* args)
{
    assert(name && path && args);
    qsf_service_t* s = qsf_malloc(sizeof(qsf_service_t));
    memset(s, 0, sizeof(*s));
    strncpy(s->name, name, sizeof(s->name));
    strncpy(s->path, path, sizeof(s->path));
    strncpy(s->args, args, sizeof(s->args));

    qsf_service_t** pph = &service_context.list;
    while (*pph)
    {
        pph = &(*pph)->next;
    }
    *pph = s;
    service_context.count++;
    return s;
}

// load Lua path and Lua cpath
static void load_service_path(lua_State* L)
{
    char chunk[256];
    const char* path = qsf_getenv("lua_path");
    if (strlen(path) > 0)
    {
        snprintf(chunk, sizeof(chunk), "package.path = package.path .. ';' .. '%s'", path);
        int r = luaL_dostring(L, chunk);
        qsf_assert(r == 0, "%s", lua_tostring(L, -1));
    }
    const char* cpath = qsf_getenv("lua_cpath");
    if (strlen(cpath) > 0)
    {
        snprintf(chunk, sizeof(chunk), "package.cpath = package.cpath .. ';' .. '%s'", cpath);
        int r = luaL_dostring(L, chunk);
        qsf_assert(r == 0, "%s", lua_tostring(L, -1));
    }
}

inline uint32_t rdrand(void)
{
    unsigned int value = 0;
#ifdef _WIN32
    rand_s(&value);
#else
    int retry = 100;
    while (__builtin_ia32_rdrand32_step(&value) == 0)
    {
        if (--retry == 0)
            break;
    }
#endif
    return value;
}

static void init_service(qsf_service_t* s)
{
    uint32_t seed = rdrand();
    srand(seed);
#ifndef _WIN32
    srandom(seed);
#endif

    lua_State* L = luaL_newstate();
    luaL_openlibs(L);
    load_service_path(L);
    initlibs(L);
    lua_pushlightuserdata(L, s); // thus `s` cannot be moved before lua_close()
    lua_setfield(L, LUA_REGISTRYINDEX, "mq_ctx");
    s->L = L;
    s->dealer = qsf_create_dealer(s->name);
    assert(s->dealer);
}

static void cleanup_service(qsf_service_t* s)
{
    qsf_log("service [%s] exit.\n", s->name);
    if (s->dealer)
    {
        zmq_close(s->dealer);
    }
    if (s->L)
    {
        lua_close(s->L);
    }
    uv_mutex_lock(&service_context.mutex);
    remove_from_service_list(s);
    uv_mutex_unlock(&service_context.mutex);
}

// execute service's lua code
static int run_service(qsf_service_t* s)
{
    lua_State* L = s->L;
    assert(L);
    int r = luaL_loadfile(L, s->path);
    if (r != 0)
    {
        qsf_log("%s: %s\n", s->name, lua_tostring(L, -1));
        return 1;
    }
    lua_pushstring(L, s->args);
    r = lua_pcall(L, 1, 0, 0);
    if (r != 0)
    {
        qsf_log("%s: %s\n", s->name, lua_tostring(L, -1));
        return 2;
    }
    return 0;
}

static void service_thread_callback(void* args)
{
    assert(args != NULL);
    qsf_service_t* s = (qsf_service_t*)args;
    init_service(s);
    run_service(s);
    cleanup_service(s);
}

int qsf_create_service(const char* name, const char* path, const char* args)
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
    
    uv_mutex_lock(&service_context.mutex);
    qsf_service_t* s = find_from_service_list(name);
    if (s != NULL)
    {
        uv_mutex_unlock(&service_context.mutex);
        return 3; // service already exist
    }
    s = create_from_service_list(name, path, args);
    int r = uv_thread_create(&s->thread, service_thread_callback, s);
	if (r < 0)
	{
		remove_from_service_list(s);
	}
    uv_mutex_unlock(&service_context.mutex);
    return r;
}

void qsf_service_send(qsf_service_t* s,
                      const char* name, int len, 
                      const char* data, int size)
{
    assert(s && name && len && data && size);

    int r = zmq_send(s->dealer, name, len, ZMQ_SNDMORE);
    qsf_assert(r == len, "send zmq dealer identity failed.");
    r = zmq_send(s->dealer, data, size, 0);
    qsf_assert(r == size, "send dealer message failed.");
}

int qsf_service_recv(struct qsf_service_s* s, 
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

const char* qsf_service_name(qsf_service_t* s)
{
    assert(s);
    return s->name;
}

int qsf_service_init()
{
    int r = uv_mutex_init(&service_context.mutex);
    if (r < 0)
    {
        qsf_log("service: uv_mutex_init() failed.\n");
        return r;
    }

    service_context.count = 0;
    service_context.list = NULL;
    return 0;
}

void qsf_service_exit()
{
	assert(service_context.count == 0);
    uv_mutex_destroy(&service_context.mutex);
}
