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

int qsf_create_service(const char* name, const char* path, const char* args);

int qsf_service_init();
void qsf_service_exit();

