// Copyright (C) 2014-2015 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#pragma once

#include "qsf.h"
#include <uv.h>

#ifndef MAX_PATH
#define MAX_PATH    256
#endif

// max dealer identity size
#define MAX_ID_LENGTH       16
#define MAX_ARG_LENGTH      512

struct qsf_service_s;
typedef struct qsf_service_s qsf_service_t;

// message handler
typedef int (*service_recv_handler)(void* ud, const char*, size_t, const char*, size_t);

// create a new servce
int qsf_create_service(const char* name, const char* path, const char* args);

// send message to another service
void qsf_service_send(qsf_service_t* s,
                      const char* name, size_t len, 
                      const char* data, size_t size);

// recv message from peer service
int qsf_service_recv(qsf_service_t* s,
                     service_recv_handler func, 
                     int nowait, void* ud);

// name of current service
const char* qsf_service_name(qsf_service_t* s);


int qsf_service_init();
void qsf_service_exit();

