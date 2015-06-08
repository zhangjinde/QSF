// Copyright (C) 2014-2015 chenqiang@chaoyuehudong.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#pragma once

#include <stdint.h>

struct uv_loop_s;
struct qsf_net_client_s;
typedef struct qsf_net_client_s qsf_net_client_t;


// callback typedef
typedef void(*c_connect_cb)(int, void* ud);
typedef void(*c_read_cb)(int, const char*, uint16_t, void*);

// create a client instance
qsf_net_client_t* qsf_create_net_client(struct uv_loop_s* loop);

// start connect to server
int qsf_net_client_connect(qsf_net_client_t* c, const char* host, int port, c_connect_cb cb);

// start read message
int qsf_net_client_read(qsf_net_client_t* c, c_read_cb cb);

// write message
int qsf_net_client_write(qsf_net_client_t* c, const char* data, uint16_t size);

// close this client
void qsf_net_client_close(qsf_net_client_t* c);

void qsf_net_client_shutdown(qsf_net_client_t* c);

// user-defined data
void qsf_net_client_set_udata(qsf_net_client_t* c, void* udata);
void* qsf_net_client_get_udata(qsf_net_client_t* c);
