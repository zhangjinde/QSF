// Copyright (C) 2014-2015 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#pragma once

#include "qsf_malloc.h"
#include "qsf_log.h"
#include "qsf_env.h"
#include "qsf_service.h"

#ifndef MAX_PATH
#define MAX_PATH    260
#endif

// start qsf framework with a config file
int qsf_start(const char* file);

// create a zmq dealer object connected to router
void* qsf_create_dealer(const char* name);

// global zmq context object
void* qsf_zmq_context(void);