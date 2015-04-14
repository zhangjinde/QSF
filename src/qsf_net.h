// Copyright (C) 2014-2015 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#include <stdint.h>
#include <uv.h>

#define MAX_FRAME_SIZE              (1024*8)
#define MAX_PACKET_SIZE             (1024*32)
#define DEFAULT_MAX_CONN            5000
#define DEFAULT_HEARTBEAT           10
#define DEFAULT_HEARTBEAT_CHECK     5

struct qsf_tcp_server_s;

typedef void(*qsf_read_callbak)(int, uint32_t, const char*, int);

struct qsf_tcp_server_s* 
qsf_create_tcp_server(uv_loop_t* loop,
                      uint32_t max_connection, 
                      uint32_t max_heart_beat, 
                      uint32_t heart_beat_check,
                      uint32_t default_buf_size);

void qsf_tcp_server_destroy(struct qsf_tcp_server_s* s);

int qsf_tcp_server_start(struct qsf_tcp_server_s* s, 
                         const char* host, 
                         uint16_t port,
                         qsf_read_callbak cb);

void qsf_tcp_server_send(struct qsf_tcp_server_s* s, 
                         uint32_t serial, 
                         const void* data,
                         size_t size);

int qsf_net_init();
void qsf_net_exit();