// Copyright (C) 2014-2015 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#pragma once

#include "qsf_malloc.h"
#include "qsf_log.h"
#include "qsf_env.h"
#include "qsf_trace.h"
#include "qsf_service.h"
#include "qsf_version.h"

#if defined(_MSC_VER)
# define QSF_TLS    __declspec(thread)
#elif defined(__GNUC__) || defined(__clang__)
# define QSF_TLS    __thread
#else
# error cannot define platform specific thread local storage
#endif

#define QSF_MAX(a, b)   (a) > (b) ? (a) : (b)
#define QSF_MIN(a, b)   (a) < (b) ? (a) : (b)

#ifndef MAX_PATH
#define MAX_PATH    260
#endif


// start qsf framework with a config file
int qsf_start(const char* file);

// create a zmq dealer object connected to router
void* qsf_create_dealer(const char* name);

// global zmq context object
void* qsf_zmq_context(void);

// qsf version info, <major.minor.patch>
void qsf_version(int* major, int* minor, int* patch);