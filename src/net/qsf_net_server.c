// Copyright (C) 2014-2015 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#include "qsf_net_server.h"
#include <time.h>
#include <string.h>
#include <assert.h>
#include <uv.h>
#include "uthash.h"
#include "qsf.h"
#include "qsf_net_def.h"

#define DEFAULT_SERIAL_NUMBER   1000
#define DEFAULT_RECV_BUF_SIZE   128

#pragma pack(push, 4)
// an session object present a TCP connection
typedef struct qsf_net_session_s
{
    UT_hash_handle      hh;                 // hash table entry
    uint32_t            serial;             // serial no.
    qsf_net_server_t*   server;             // server pointer
    uint64_t            last_recv_time;     // last recv time
    uv_tcp_t            handle;             // tcp handle
    struct sockaddr_in  peer_addr;          // peer address
    uint32_t            buf_size;           // recv buffer size
    uint16_t            recv_bytes;         // total recieved bytes
    uint16_t            head_size;          // head bytes
    char*               recv_buf;           // recv buffer
}qsf_net_session_t;

struct qsf_net_server_s
{
    uint32_t    max_connection;     // max alive connections
    uint16_t    max_heart_beat;     // max heart beat seconds
    uint16_t    heart_beat_check;   // max check heart beat seconds
    int         stopped;            // is server stopped
    uint32_t    next_serial;        // next session serial no.
    void*       udata;              // user data pointer
    s_read_cb   on_read;            // read handler
    uv_tcp_t    acceptor;           // tcp accept handle
    uv_timer_t  timer;              // timer handle
    qsf_net_session_t* session_map; // session hash map
};
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct write_buffer_s
{
    uv_write_t  req;        // request handle
    uv_buf_t    buf;        // buffer object
    uint16_t    size;       // data size
    char        data[];     // data buffer
}write_buffer_t;
#pragma pack(pop)

// set a unique serial number to this session
static void set_session_serial(qsf_net_server_t* server, qsf_net_session_t* session)
{
    assert(server);
    uint32_t serial = server->next_serial;
    for (;;)
    {
        qsf_net_session_t* s = NULL;
        HASH_FIND_INT(server->session_map, &serial, s);
        if (s == NULL)
            break;
        serial++;
    }
    session->serial = serial;
    server->next_serial = serial + 1;
}

static void on_session_close(uv_handle_t* handle)
{
    qsf_net_session_t* session = handle->data;
    assert(session);
    qsf_free(session->recv_buf);
    qsf_free(session);
}

static void session_destroy(qsf_net_session_t* session)
{
    uv_handle_t* handle = (uv_handle_t*)&session->handle;
    uv_read_stop((uv_stream_t*)&session->handle);
    if (!uv_is_closing(handle))
    {
        uv_close(handle, on_session_close);
    }
}

// create an session
static qsf_net_session_t* session_create(uv_loop_t* loop, qsf_net_server_t* server)
{
    assert(loop && server);
    qsf_net_session_t* session = qsf_malloc(sizeof(qsf_net_session_t));
    memset(session, 0, sizeof(*session));
    int r = uv_tcp_init(loop, &session->handle);
    qsf_assert(r == 0, "session: uv_tcp_init() failed");
    session->handle.data = session;
    session->server = server;
    session->last_recv_time = uv_now(loop);
    session->buf_size = DEFAULT_RECV_BUF_SIZE;
    session->recv_buf = qsf_malloc(DEFAULT_RECV_BUF_SIZE);
    return session;
}

static void on_session_alloc(uv_handle_t* handle, size_t size, uv_buf_t* buf)
{
    qsf_net_session_t* s = handle->data;
    assert(s);
    uint16_t bytes = s->recv_bytes;
    buf->base = s->recv_buf + bytes;
    if (s->head_size == 0) // header not filled
    {
        buf->len = sizeof(s->head_size) - bytes;
    }
    else // read body content
    {
        buf->len = s->head_size - bytes;
    }
}

static void on_session_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf)
{
    qsf_net_session_t* session = stream->data;
    qsf_net_server_t* server = session->server;
    s_read_cb cb = server->on_read;
    assert(session && server && cb);
    if (nread <= 0)
    {
        if (nread < 0)
        {
            const char* msg = uv_strerror((int)nread);
            cb((int)nread, session->serial, msg, (uint16_t)strlen(msg), server->udata);
            HASH_DEL(server->session_map, session);
            session_destroy(session);
        }
        return;
    }
    session->last_recv_time = uv_now(stream->loop);
    session->recv_bytes += (uint16_t)nread;
    uint16_t size = session->head_size;
    if (size == 0) // header not full-filled
    {
        if (session->recv_bytes == sizeof(size))
        {
            memcpy(&size, session->recv_buf, sizeof(size));
            session->head_size = ntohs(size); // network order
            session->recv_bytes = 0;
            if (session->head_size > session->buf_size) // extending recv buffer
            {
                qsf_free(session->recv_buf);
                session->recv_buf = qsf_malloc(session->head_size);
                session->buf_size = session->head_size;
            }
        }
    }
    else // fill body content
    {
        if (session->recv_bytes == size)
        {
            session->recv_bytes = 0;
            session->head_size = 0;
            cb(0, session->serial, session->recv_buf, size, server->udata);
        }
    }
}

// on client accept
static void on_connection(uv_stream_t* stream, int err)
{
    if (err != 0)
    {
        qsf_log("listen failed, %d: %s\n", err, uv_strerror(err));
        return;
    }
    qsf_net_server_t* server = stream->data;
    assert(server);
    if (server->stopped)
    {
        return;
    }
    uint32_t count = HASH_COUNT(server->session_map);
    if (count >= server->max_connection)
    {
        return; // max connection count limit
    }
    qsf_net_session_t* session = session_create(stream->loop, server);
    uv_stream_t* handle = (uv_stream_t*)&session->handle;
    int r = uv_accept(stream, handle);
    if (r < 0)
    {
        session_destroy(session);
        qsf_log("accept failed, %d: %s\n", r, uv_strerror(r));
        return;
    }
    int namelen = sizeof(session->peer_addr);
    r = uv_tcp_getpeername(&session->handle, (struct sockaddr*)&session->peer_addr, &namelen);
    if (r < 0)
    {
        session_destroy(session);
        qsf_log("getpeername failed, %d: %s\n", r, uv_strerror(r));
        return;
    }
    r = uv_read_start(handle, on_session_alloc, on_session_read);
    if (r < 0)
    {
        session_destroy(session);
        qsf_log("getpeername failed, %d: %s\n", r, uv_strerror(r));
        return;
    }
    set_session_serial(server, session);
    HASH_ADD_INT(server->session_map, serial, session);
}

// heart beat checking
static void hearbeat_timer_cb(uv_timer_t* timer)
{
    qsf_net_server_t* server = timer->data;
    assert(server);
    qsf_net_session_t* session = NULL;
    qsf_net_session_t* tmp = NULL;
    uint64_t now = uv_now(timer->loop);
    uint64_t max_expire = server->max_heart_beat * 1000;
    HASH_ITER(hh, server->session_map, session, tmp)
    {
        if (now - session->last_recv_time > max_expire)
        {
            const char* msg = "session timeout";
            server->on_read(NET_ERR_TIMEOUT, session->serial, msg, (uint16_t)strlen(msg), server->udata);
            HASH_DEL(server->session_map, session);
            session_destroy(session);
        }
    }
}

qsf_net_server_t* qsf_create_net_server(uv_loop_t* loop,
                                        uint32_t max_connection,
                                        uint16_t max_heart_beat,
                                        uint16_t heart_beat_check)
{
    assert(loop);
    qsf_net_server_t* server = qsf_malloc(sizeof(qsf_net_server_t));
    qsf_assert(server != NULL, "create_server() failed.");
    memset(server, 0, sizeof(*server));
    int r = uv_tcp_init(loop, &server->acceptor);
    qsf_assert(r == 0, "uv_tcp_init() failed."); 
    r = uv_timer_init(loop, &server->timer);
    qsf_assert(r == 0, "uv_tcp_init() failed.");
    
    server->acceptor.data = server;
    server->timer.data = server;
    server->max_connection = max_connection;
    server->max_heart_beat = max_heart_beat;
    server->heart_beat_check = heart_beat_check;
    server->next_serial = DEFAULT_SERIAL_NUMBER;
    return server;
}

void qsf_net_server_destroy(qsf_net_server_t* s)
{
    assert(s);
    qsf_net_server_stop(s);
    qsf_free(s);
}

int qsf_net_server_start(qsf_net_server_t* s,
                         const char* host, 
                         int port,
                         s_read_cb on_read)
{
    assert(s && host && on_read);
    s->on_read = on_read;
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
    r =  uv_listen((uv_stream_t*)&s->acceptor, SOMAXCONN, on_connection);
    if (r < 0)
    {
        return r;
    }
    uint64_t timeout = s->heart_beat_check * 1000;
    r = uv_timer_start(&s->timer, hearbeat_timer_cb, timeout, timeout);
    if (r < 0)
    {
        return r;
    }
    return 0;
}

void qsf_net_server_stop(qsf_net_server_t* s)
{
    assert(s);
    qsf_net_session_t* session = NULL;
    qsf_net_session_t* tmp = NULL;
    s->stopped = 1;
    HASH_ITER(hh, s->session_map, session, tmp)
    {
        HASH_DEL(s->session_map, session);
        session_destroy(session);
    }
    uv_timer_stop(&s->timer);
    uv_handle_t* acceptor = (uv_handle_t*)&s->acceptor;
    uv_handle_t* timer = (uv_handle_t*)&s->timer;
    if (!uv_is_closing(timer))
    {
        uv_close(timer, NULL);
    }
    if (!uv_is_closing(acceptor))
    {
        uv_close(acceptor, NULL);
    }
}

static void session_write_cb(uv_write_t* req, int err)
{
    write_buffer_t* buffer = req->data;
    qsf_free(buffer);
}

int do_session_write(qsf_net_session_t* session, const void* data, uint16_t size)
{
    assert(session && data && size);
    write_buffer_t* buffer = qsf_malloc(sizeof(write_buffer_t) + size);
    buffer->req.data = buffer;
    buffer->size = htons(size); //network order
    memcpy(buffer->data, data, size);
    buffer->buf.base = (char*)&buffer->size; // memory layout dependency
    buffer->buf.len = sizeof(size) + size;
    int r = uv_write(&buffer->req, (uv_stream_t*)&session->handle, &buffer->buf,
        1, session_write_cb);
    if (r < 0)
    {
        qsf_free(buffer);
        return r;
    }
    return 0;
}

int qsf_net_server_write(qsf_net_server_t* s,
                         uint32_t serial,
                         const void* data,
                         uint16_t size)
{
    assert(s && data && size);
    qsf_net_session_t* session = NULL;
    HASH_FIND_INT(s->session_map, &serial, session);
    if (session == NULL)
    {
        return -1;
    }
    return do_session_write(session, data, size);
}

int qsf_net_server_write_all(qsf_net_server_t* s, const void* data, uint16_t size)
{
    assert(s && data && size);
    qsf_net_session_t* session = NULL;
    qsf_net_session_t* tmp = NULL;
    HASH_ITER(hh, s->session_map, session, tmp)
    {
        do_session_write(session, data, size);
    }
    return 0;
}

static void on_session_shutdown(uv_shutdown_t* req, int err)
{
    qsf_net_session_t* session = req->data;
    assert(session);
    qsf_free(req);
}

void qsf_net_server_shutdown(qsf_net_server_t* s, uint32_t serial)
{
    assert(s);
    qsf_net_session_t* session = NULL;
    HASH_FIND_INT(s->session_map, &serial, session);
    if (session == NULL)
    {
        return;
    }
    uv_read_stop((uv_stream_t*)&session->handle);
    uv_shutdown_t* req = qsf_malloc(sizeof(uv_shutdown_t));
    req->data = session;
    int r = uv_shutdown(req, (uv_stream_t*)&session->handle, on_session_shutdown);
    if (r < 0)
    {
        qsf_free(req);
        qsf_log("shutdown session failed: %d\n", session->serial);
    }
}

void qsf_net_server_close(qsf_net_server_t* s, uint32_t serial)
{
    assert(s);
    qsf_net_session_t* session = NULL;
    HASH_FIND_INT(s->session_map, &serial, session);
    if (session)
    {
        HASH_DEL(s->session_map, session);
        session_destroy(session);
    }
}

int qsf_net_server_session_address(qsf_net_server_t* s,
                                   uint32_t serial,
                                   char* address,
                                   int length)
{
    assert(s && address && length);
    qsf_net_session_t* session = NULL;
    HASH_FIND_INT(s->session_map, &serial, session);
    if (session == NULL)
    {
        return -1;
    }
    return uv_ip4_name(&session->peer_addr, address, length);
}

int qsf_net_server_size(qsf_net_server_t* s)
{
    assert(s);
    return HASH_COUNT(s->session_map);
}

void qsf_net_set_server_udata(qsf_net_server_t* s, void* ud)
{
    assert(s);
    s->udata = ud;
}

void* qsf_net_get_server_udata(qsf_net_server_t* s)
{
    assert(s);
    return s->udata;
}
