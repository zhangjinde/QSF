// Copyright (C) 2014-2015 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#pragma once

#include <stdint.h>

struct qsf_service_s;
typedef struct qsf_service_s qsf_service_t;

// message handler
typedef int(*msg_recv_handler)(void* ud, const char*, int, const char*, int);

// create a new servce
int qsf_create_service(const char* name, const char* path, const char* args);

// send message to another service
void qsf_service_send(qsf_service_t* s,
                      const char* name, int len, 
                      const char* data, int size);

// recv message from peer service
int qsf_service_recv(qsf_service_t* s,
                     msg_recv_handler func, 
                     int nowait, void* ud);

// name of current service
const char* qsf_service_name(qsf_service_t* s);

// adapt random integer number closed range to [0, max)
uint32_t qsf_service_rand32(qsf_service_t* s, uint32_t max);

// random float number open range (0.0, 1.0)
float qsf_service_randf(qsf_service_t* s);

int qsf_service_init();
void qsf_service_exit();

