// Copyright (C) 2014-2015 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#pragma once

#include "qsf.h"
#include <uv.h>

#ifndef MAX_PATH
#define MAX_PATH    256
#endif

// Max dealer identity size
#define MAX_ID_LENGTH       16
#define MAX_ARG_LENGTH      512

struct qsf_service_s;

typedef int (*service_recv_handler)(void* ud, const char*, size_t, const char*, size_t);

// Create a new servce
int qsf_create_service(const char* name, const char* path, const char* args);

void qsf_service_send(struct qsf_service_s* s,
                      const char* name, size_t len, 
                      const char* data, size_t size);

int qsf_service_recv(struct qsf_service_s* s, 
                     service_recv_handler func, 
                     int nowait, void* ud);

const char* qsf_service_name(struct qsf_service_s* s);

int qsf_service_init();
void qsf_service_exit();

