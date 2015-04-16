// Copyright (C) 2014-2015 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#pragma once

#include <stdint.h>

struct qsf_net_client_s;
typedef struct qsf_net_client_s qsf_net_client_t;

struct uv_loop_s;
typedef struct uv_loop_s uv_loop_t;

// callback typedef
typedef void(*c_connect_cb)(int, void* ud);
typedef void(*c_read_cb)(int, const char*, uint16_t, void* ud);

// create a client instance
qsf_net_client_t* qsf_create_net_client(uv_loop_t* loop);

// start connect to server
int qsf_net_client_connect(qsf_net_client_t* c, const char* host, int port, c_connect_cb cb);

// start read message
int qsf_net_client_read(qsf_net_client_t* c, c_read_cb cb);

// write message
int qsf_net_client_write(qsf_net_client_t* c, const char* data, uint16_t size);

// close this client
void qsf_net_client_close(qsf_net_client_t* c);

// user-defined data
void qsf_net_client_set_udata(qsf_net_client_t* c, void* udata);
void* qsf_net_client_get_udata(qsf_net_client_t* c);