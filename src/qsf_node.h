// Copyright (C) 2014-2015 chenqiang@chaoyuehudong.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#pragma once

#include <stdint.h>
#include <uv.h>

struct qsf_node_s;
typedef struct qsf_node_s qsf_node_t;
typedef struct lua_State lua_State;


#define QSF_NODE_TAG_VALUE_GOOD 0xabadcafe
#define QSF_NODE_TAG_VALUE_BAD  0xdeadbeef


// message handler
typedef int(*msg_recv_handler)(void* ud, const char*, int, const char*, int);

// create a new service
int qsf_create_node(const char* name, const char* path, const char* args);

int qsf_node_check_tag(qsf_node_t* s);

// send message to another service
void qsf_node_send(qsf_node_t* s,
                   const char* name, int len, 
                   const char* data, int size);

// recv message from peer service
int qsf_node_recv(qsf_node_t* s,
                  msg_recv_handler func, 
                  int nowait, void* ud);

int qsf_node_run(qsf_node_t* s);

uv_loop_t* qsf_node_loop(qsf_node_t* s);

// name of current service
const char* qsf_node_name(qsf_node_t* s);

int qsf_trace_pcall(lua_State* L, int narg);

int qsf_node_init();
void qsf_node_exit();

