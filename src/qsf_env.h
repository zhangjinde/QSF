// Copyright (C) 2014-2015 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#pragma once

#include <stdint.h>

// get qsf environment vairable
const char* qsf_getenv(const char* key);

// get qsf environment vairable with integer type
int64_t qsf_getenv_int(const char* key);

// set qsf environment vairable
void qsf_setenv(const char* key, const char* value);


int qsf_env_init(const char* file);
void qsf_env_exit(void);
