// Copyright (C) 2014-2015 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#include "qsf_net_client.h"
#include <assert.h>
#include <string.h>
#include <uv.h>
#include "qsf.h"

#define DEFAULT_RECV_BUF_SIZE   512

#pragma pack(push, 4)
struct qsf_net_client_s
{
    uv_tcp_t        handle;         // uv tcp handle
    c_connect_cb    on_connect;     // connect callback
    c_read_cb       on_read;        // read callback
    void*           udata;          // user defined data pointer
    uint32_t        buf_size;       // recv buffer size
    uint16_t        recv_bytes;     // recv bytes
    uint16_t        head_size;      // header size
    char*           recv_buf;       // recv data buffer
};
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct write_buffer_s
{
    uv_write_t  req;        // write request
    uv_buf_t    buf;        // buffer object
    uint16_t    size;       // buffer size
    char        data[];     // buffer data
}write_buffer_t;
#pragma pack(pop)


static void on_client_close(uv_handle_t* handle)
{
    qsf_net_client_t* c = handle->data;
    assert(c);
    qsf_free(c);
}

static void on_client_connect(uv_connect_t* req, int status)
{
    qsf_net_client_t* c = req->data;
    assert(c->on_connect);
    c->on_connect(status, c->udata);
}

static void on_client_alloc(uv_handle_t* handle, size_t size, uv_buf_t* buf)
{
    qsf_net_client_t* c = handle->data;
    assert(c);
    uint16_t bytes = c->recv_bytes;
    buf->base = c->recv_buf + bytes;
    if (c->head_size == 0) // header not filled
    {
        buf->len = sizeof(c->head_size) - bytes;
    }
    else // read body content
    {
        buf->len = c->head_size - bytes;
    }
}

static void on_client_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf)
{
    qsf_net_client_t* c = stream->data;
    assert(c && c->on_read);
    if (nread <= 0)
    {
        if (nread < 0)
        {
            const char* msg = uv_strerror((int)nread);
            c->on_read((int)nread, msg, (uint16_t)strlen(msg), c->udata);
        }
        return;
    }
    c->recv_bytes += (uint16_t)nread;
    uint16_t size = c->head_size;
    if (size == 0) // header not full-filled
    {
        if (c->recv_bytes == sizeof(size))
        {
            memcpy(&size, c->recv_buf, sizeof(size));
            c->head_size = ntohs(size); // network order
            c->recv_bytes = 0;
            if (c->head_size > c->buf_size)
            {
                qsf_free(c->recv_buf);
                c->recv_buf = qsf_malloc(c->head_size);
                c->buf_size = c->head_size;
            }
        }
    }
    else // fill body content
    {
        if (c->recv_bytes == size)
        {
            c->recv_bytes = 0;
            c->head_size = 0;
            c->on_read(0, c->recv_buf, size, c->udata);
        }
    }
}

static void client_write_cb(uv_write_t* req, int err)
{
    write_buffer_t* buffer = req->data;
    qsf_free(buffer);
}

qsf_net_client_t* qsf_create_net_client(uv_loop_t* loop)
{
    assert(loop);
    qsf_net_client_t* c = qsf_malloc(sizeof(qsf_net_client_t));
    memset(c, 0, sizeof(*c));
    int r = uv_tcp_init(loop, &c->handle);
    qsf_assert(r == 0, "uv_tcp_init() failed.");
    c->handle.data = c;
    c->buf_size = DEFAULT_RECV_BUF_SIZE;
    c->recv_buf = qsf_malloc(DEFAULT_RECV_BUF_SIZE);
    return c;
}

int qsf_net_client_connect(qsf_net_client_t* c, const char* host, int port, c_connect_cb cb)
{
    assert(c && host && cb);
    struct sockaddr_in addr;
    int r = uv_ip4_addr(host, port, &addr);
    if (r < 0)
    {
        return r;
    }
    c->on_connect = cb;
    uv_connect_t* req = qsf_malloc(sizeof(uv_connect_t));
    req->data = c;
    r = uv_tcp_connect(req, &c->handle, (const struct sockaddr*)&addr, on_client_connect);
    if (r < 0)
    {
        qsf_free(req);
    }
    return r;
}

int qsf_net_client_read(qsf_net_client_t* c, c_read_cb cb)
{
    assert(c && cb);
    c->on_read = cb;
    return uv_read_start((uv_stream_t*)&c->handle, on_client_alloc, on_client_read);
}

int qsf_net_client_write(qsf_net_client_t* c, const char* data, uint16_t size)
{
    assert(c && data && size);
    write_buffer_t* buffer = qsf_malloc(sizeof(write_buffer_t) + size);
    buffer->req.data = buffer;
    buffer->size = htons(size); //network order
    memcpy(buffer->data, data, size);
    buffer->buf.base = (char*)&buffer->size; // memory layout dependency
    buffer->buf.len = sizeof(size) + size;
    int r = uv_write(&buffer->req, (uv_stream_t*)&c->handle, &buffer->buf,
        1, client_write_cb);
    if (r < 0)
    {
        qsf_free(buffer);
    }
    return r;
}

void qsf_net_client_close(qsf_net_client_t* c)
{
    assert(c);
    uv_handle_t* handle = (uv_handle_t*)&c->handle;
    if (!uv_is_closing(handle))
    {
        uv_close(handle, on_client_close);
    }
}

void qsf_net_client_set_udata(qsf_net_client_t* c, void* udata)
{
    assert(c);
    c->udata = udata;
}

void* qsf_net_client_get_udata(qsf_net_client_t* c)
{
    assert(c);
    return c->udata;
}
