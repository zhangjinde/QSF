// Copyright (C) 2014-2015 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#include "qsf_net.h"
#include <time.h>
#include <assert.h>
#include "qsf_malloc.h"

typedef struct qsf_tcp_session_s
{
    RB_ENTRY(qsf_tcp_session_s) tree_entry;
    uint32_t serial;
    uv_tcp_t handle;
    time_t last_recv_time;
    char addr[40];
    uint16_t closed;
    uint16_t bytes;
    char buf[1];
}qsf_tcp_session_t;

static int tcp_session_compare(qsf_tcp_session_t* a, qsf_tcp_session_t* b)
{
    assert(a && b);
    return a->serial > b->serial;
}

RB_HEAD(qsf_tcp_session_tree_s, qsf_tcp_session_s);
RB_GENERATE_STATIC(qsf_tcp_session_tree_s, qsf_tcp_session_s, tree_entry, tcp_session_compare);

typedef struct qsf_tcp_server_s
{
    uint32_t max_connection;
    uint32_t max_heart_beat;
    uint32_t heart_beat_check;
    uint32_t default_buf_size;
    uint32_t next_serial;
    qsf_read_callbak on_read;
    uv_tcp_t acceptor;
    struct qsf_tcp_session_tree_s session_map;
}qsf_tcp_server_t;


static qsf_tcp_session_t* session_create(uv_loop_t* loop, uint32_t buf_size)
{
    qsf_tcp_session_t* session = qsf_malloc(sizeof(qsf_tcp_session_t) + buf_size);
    int r = uv_tcp_init(loop, &session->handle);
    if (r < 0)
    {
        qsf_free(session);
        return NULL;
    }
    session->handle.data = session;
    session->last_recv_time = 0;
    session->closed = 0;
    session->addr[0] = '\0';
    session->bytes = 0;
    return session;
}

static void session_destroy(qsf_tcp_session_t* session)
{
    uv_handle_t* handle = (uv_handle_t*)&session->handle;
    if (!uv_is_closing(handle))
    {
        uv_close(handle, NULL);
    }
    qsf_free(session);
}

static uint32_t next_session_serial(qsf_tcp_server_t* server, qsf_tcp_session_t* session)
{
    assert(server && session);
    session->serial = server->next_serial;
    while (RB_FIND(qsf_tcp_session_tree_s, &server->session_map, session))
    {
        session->serial++;
    }
    server->next_serial = session->serial + 1;
}



static void connection_cb(uv_stream_t* handle, int err)
{
    qsf_tcp_server_t* server = handle->data;
    qsf_tcp_session_t* session = session_create(server->acceptor.loop, server->default_buf_size);
    int r = uv_accept(handle, (uv_stream_t*)&session->handle);
    if (r < 0)
    {
        session_destroy(session);
        return;
    }
}


qsf_tcp_server_t* qsf_create_tcp_server(uv_loop_t* loop,
                                        uint32_t max_connection,
                                        uint32_t max_heart_beat,
                                        uint32_t heart_beat_check,
                                        uint32_t default_buf_size)
{
    qsf_tcp_server_t* server = qsf_malloc(sizeof(qsf_tcp_server_t));
    int r = uv_tcp_init(loop, &server->acceptor);
    if (r < 0)
    {
        qsf_free(server);
        return NULL;
    }
    server->acceptor.data = server;
    server->max_connection = max_connection;
    server->max_heart_beat = max_heart_beat;
    server->heart_beat_check = heart_beat_check;
    server->default_buf_size = default_buf_size;
    server->next_serial = 1000;
    server->on_read = NULL;
    return server;
}

void qsf_tcp_server_destroy(struct qsf_tcp_server_s* s)
{
    uv_handle_t* handle = (uv_handle_t*)&s->acceptor;
    if (!uv_is_closing(handle))
    {
        uv_close(handle, NULL);
    }
    qsf_free(s);
}

int qsf_tcp_server_start(struct qsf_tcp_server_s* s, 
                         const char* host, 
                         uint16_t port,
                         qsf_read_callbak cb)
{
    assert(s && host && cb);
    s->on_read = cb;
    struct sockaddr_in addr;
    int r = uv_ip4_addr(host, port, &addr);
    if (r < 0)
    {
        return r;
    }
    r = uv_tcp_bind(&s->acceptor, (const struct sockaddr*)&addr, 0);
    if (r < 0)
    {
        return r;
    }
    r = uv_listen((uv_stream_t*)&s->acceptor, SOMAXCONN, connection_cb);
    return 0;
}

