// Copyright (C) 2014-2015 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#pragma once

#include <stdint.h>

struct qsf_net_server_s;
typedef struct qsf_net_server_s qsf_net_server_t;

struct uv_loop_s;
typedef struct uv_loop_s uv_loop_t;

// callbacks
typedef void(*s_read_cb)(int, uint32_t, const char*, uint16_t, void* ud);

// create an net server instance
struct qsf_net_server_s*
qsf_create_net_server(uv_loop_t* loop,
                      uint16_t max_connection, 
                      uint16_t max_heart_beat,
                      uint16_t heart_beat_check,
                      uint16_t max_buffer_size);

// destroy this instance
void qsf_net_server_destroy(struct qsf_net_server_s* s);

// start net server
int qsf_net_server_start(qsf_net_server_t* s,
                         const char* host, 
                         int port,
                         s_read_cb on_read);

// stop net server
void qsf_net_server_stop(qsf_net_server_t* s);

// send message to specified session
int qsf_net_server_write(qsf_net_server_t* s,
                         uint32_t serial, 
                         const void* data,
                         uint16_t size);


// send message to all sessions
int qsf_net_server_write_all(qsf_net_server_t* s,
                             const void* data,
                             uint16_t size);

void qsf_net_server_shutdown(qsf_net_server_t* s, uint32_t serial);

void qsf_net_server_close(qsf_net_server_t* s, uint32_t serial);

int qsf_net_server_session_address(qsf_net_server_t* s,
                                   uint32_t serial,
                                   char* address,
                                   size_t length);

// server reference
void qsf_net_set_server_udata(qsf_net_server_t* s, void* ud);
void* qsf_net_get_server_udata(qsf_net_server_t* s);


int qsf_net_init();
void qsf_net_exit();