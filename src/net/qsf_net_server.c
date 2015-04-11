// Copyright (C) 2014-2015 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#include "qsf_net_server.h"
#include <time.h>
#include <assert.h>
#include <uv.h>
#include "uthash.h"
#include "qsf.h"
#include "qsf_net_def.h"

// an session object present a TCP connection
typedef struct qsf_net_session_s
{
    UT_hash_handle      hh;
    uint32_t            serial;             // serial no.
    qsf_net_server_t*   server;             // server pointer
    time_t              last_recv_time;     //
    uv_tcp_t            handle;             // tcp handle
    struct sockaddr_in  peer_addr;          // peer address
    uint16_t            head_size;          // head bytes
    uint16_t            recv_bytes;         // total recieved bytes
    char                recv_buf[1];        // recv buffer
}qsf_net_session_t;

typedef struct write_buffer_s
{
    uv_write_t  req;                // request handle
    uv_buf_t    buf;                // buffer object
    uint16_t    size;               // data size
    char        data[1];            // data buffer
}write_buffer_t;

typedef struct shared_write_buffer_s
{
    uv_write_t  req;                // request handle
    uv_buf_t    buf;                // buffer object
    uint16_t    ref;                // reference count
    uint16_t    size;               // data size
    char        data[1];            // data buffer
}shared_write_buffer_t;

struct qsf_net_server_s
{
    uint16_t max_connection;        // max alive connections
    uint16_t max_heart_beat;        // max heart beat seconds
    uint16_t heart_beat_check;      // max check heart beat seconds
    uint16_t max_buffer_size;       // max recv buffer size for each session

    void*       udata;              // user data pointer
    uint32_t    next_serial;        // next session serial no.
    s_read_cb   on_read;            // read handler
    uv_tcp_t    acceptor;           // tcp accept handle
    uv_timer_t  timer;              // timer handle
    qsf_net_session_t* session_map; // session hash map
};

// set a unique serial no. to this session
static void set_session_serial(qsf_net_server_t* server, qsf_net_session_t* session)
{
    assert(server);
    uint32_t serial = server->next_serial;
    while (1)
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
    qsf_free(session);
}

void on_session_shutdown(uv_shutdown_t* req, int err)
{
    qsf_net_session_t* session = req->data;
    assert(session);
    qsf_free(req);
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
    size_t bytes = sizeof(qsf_net_session_t) + server->max_buffer_size;
    qsf_net_session_t* session = qsf_malloc(bytes);
    memset(session, 0, sizeof(*session));
    int r = uv_tcp_init(loop, &session->handle);
    qsf_assert(r == 0, "session: uv_tcp_init() failed");
    session->handle.data = session;
    session->server = server;
    session->last_recv_time = time(NULL);
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
    if (nread <= 0)
    {
        if (nread < 0)
        {
            const char* msg = uv_strerror(nread);
            cb(nread, session->serial, msg, strlen(msg), server->udata);
            HASH_DEL(server->session_map, session);
            session_destroy(session);
        }
        return;
    }
    session->last_recv_time = time(NULL);
    session->recv_bytes += (uint16_t)nread;
    uint16_t size = session->head_size;
    if (size == 0) // header not full-filled
    {
        if (session->recv_bytes == sizeof(size))
        {
            memcpy(&size, session->recv_buf, sizeof(size));
            session->head_size = ntohs(size); // network order
            session->recv_bytes = 0;
            if (session->head_size > server->max_buffer_size)
            {
                const char* msg = "content size limit";
                cb(nread, session->serial, msg, strlen(msg), server->udata);
                HASH_DEL(server->session_map, session);
                session_destroy(session);
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
        qsf_log("accept failed, %d: %s", err, uv_strerror(err));
        return;
    }
    qsf_net_server_t* server = stream->data;
    assert(server);

    int count = HASH_COUNT(server->session_map);
    if (count >= server->max_connection)
        return ;

    qsf_net_session_t* session = session_create(stream->loop, server);
    uv_stream_t* handle = (uv_stream_t*)&session->handle;
    int r = uv_accept(stream, handle);
    if (r < 0)
    {
        session_destroy(session);
        return;
    }
    int namelen = sizeof(session->peer_addr);
    r = uv_tcp_getpeername(&session->handle, (struct sockaddr*)&session->peer_addr, &namelen);
    if (r < 0)
    {
        session_destroy(session);
        return;
    }
    r = uv_read_start(handle, on_session_alloc, on_session_read);
    if (r < 0)
    {
        session_destroy(session);
        return;
    }
    set_session_serial(server, session);
    HASH_ADD_INT(server->session_map, serial, session);
}

static void session_write_cb(uv_write_t* req, int err)
{
    write_buffer_t* buffer = req->data;
    qsf_free(buffer);
}

static void session_shared_write_cb(uv_write_t* req, int err)
{
    shared_write_buffer_t* buffer = req->data;
    if (buffer->ref-- == 0)
    {
        qsf_free(buffer);
    }
}

// heart beat checking
static void hearbeat_timer_cb(uv_timer_t* timer)
{
    qsf_net_server_t* server = timer->data;
    assert(server);
    qsf_net_session_t* session = NULL;
    qsf_net_session_t* tmp = NULL;
    time_t now = time(NULL);
    HASH_ITER(hh, server->session_map, session, tmp)
    {
        if (now - session->last_recv_time > server->max_heart_beat)
        {
            const char* msg = "session timeout";
            server->on_read(NET_ERR_TIMEOUT, session->serial, msg, strlen(msg), server->udata);
            HASH_DEL(server->session_map, session);
            session_destroy(session);
        }
    }
}

static void on_server_close(uv_handle_t* handle)
{
    qsf_net_server_t* s = handle->data;
    assert(s);
    qsf_free(s);
}

qsf_net_server_t* qsf_create_net_server(uv_loop_t* loop,
                                        uint16_t max_connection,
                                        uint16_t max_heart_beat,
                                        uint16_t heart_beat_check,
                                        uint16_t max_buffer_size)
{
    assert(loop);
    qsf_net_server_t* server = qsf_malloc(sizeof(qsf_net_server_t));
    int r = uv_tcp_init(loop, &server->acceptor);
    qsf_assert(r == 0, "uv_tcp_init() failed.");
    r = uv_timer_init(loop, &server->timer);
    qsf_assert(r == 0, "uv_tcp_init() failed.");
    
    server->acceptor.data = server;
    server->timer.data = server;
    server->max_connection = max_connection;
    server->max_heart_beat = max_heart_beat;
    server->heart_beat_check = heart_beat_check;
    server->max_buffer_size = max_buffer_size;
    server->next_serial = 1000;
    server->on_read = NULL;
    server->session_map = NULL;
    return server;
}

void qsf_net_server_destroy(qsf_net_server_t* s)
{
    assert(s);
    qsf_net_server_stop(s);
    uv_handle_t* acceptor = (uv_handle_t*)&s->acceptor;
    uv_handle_t* timer = (uv_handle_t*)&s->timer;
    uv_timer_stop(&s->timer);
    if (!uv_is_closing(timer))
    {
        uv_close(timer, NULL);
    }
    if (!uv_is_closing(acceptor))
    {
        uv_close(acceptor, on_server_close);
    }
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
    int64_t timeout = s->heart_beat_check * 1000;
    return uv_timer_start(&s->timer, hearbeat_timer_cb, timeout, timeout);
}

void qsf_net_server_stop(qsf_net_server_t* s)
{
    assert(s);
    qsf_net_session_t* session = NULL;
    qsf_net_session_t* tmp = NULL;
    HASH_ITER(hh, s->session_map, session, tmp)
    {
        HASH_DEL(s->session_map, session);
        session_destroy(session);
    }
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
    }
    return r;
}

int qsf_net_server_write_all(qsf_net_server_t* s,
                             const void* data,
                             uint16_t size)
{
    assert(s && data && size);
    qsf_net_session_t* session = NULL;
    qsf_net_session_t* tmp = NULL;
    size_t count = HASH_COUNT(s->session_map);
    if (count == 0)
    {
        return 0;
    }
    shared_write_buffer_t* buffer = qsf_malloc(sizeof(shared_write_buffer_t) + size);
    buffer->req.data = buffer;
    buffer->ref = 0;
    buffer->size = htons(size);
    memcpy(buffer->data, data, size);
    buffer->buf.base = (char*)&buffer->size;
    buffer->buf.len = sizeof(size) + size;
    HASH_ITER(hh, s->session_map, session, tmp)
    {
        int r = uv_write(&buffer->req, (uv_stream_t*)&session->handle, &buffer->buf, 
            1, session_shared_write_cb);
        if (r == 0)
        {
            buffer->ref++;
        }
    }
    return 0;
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
    uv_shutdown_t* req = qsf_malloc(sizeof(uv_shutdown_t));
    int r = uv_shutdown(req, (uv_stream_t*)&session->handle, on_session_shutdown);
    if (r < 0)
    {
        qsf_free(req);
        qsf_log("shutdown session failed: %d", session->serial);
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
                                   size_t length)
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
