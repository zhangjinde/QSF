// Copyright (C) 2014-2015 chenqiang@chaoyuehudong.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#pragma once

#include "qsf_config.h"
#include "qsf_version.h"
#include "qsf_malloc.h"
#include "qsf_log.h"
#include "qsf_env.h"
#include "qsf_trace.h"
#include "qsf_service.h"


#define QSF_MAX(a, b)   ((a) > (b) ? (a) : (b))
#define QSF_MIN(a, b)   ((a) < (b) ? (a) : (b))

// start qsf framework with a config file
int qsf_start(const char* file);
void qsf_exit(int sig);

// create a zmq dealer object connected to router
void* qsf_create_dealer(const char* name);

// global zmq context object
void* qsf_zmq_context(void);

// qsf version info, <major.minor.patch>
void qsf_version(int* major, int* minor, int* patch);
