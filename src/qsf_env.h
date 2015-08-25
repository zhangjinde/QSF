// Copyright (C) 2014-2015 chenqiang@chaoyuehudong.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#pragma once

#include <stdint.h>

// get qsf environment string vairable
const char* qsf_getenv(const char* key, const char* deflt);

// get qsf environment integer vairable
int64_t qsf_getenv_int(const char* key, int64_t deflt);

// set qsf environment vairable
void qsf_setenv(const char* key, const char* value);

// init and exit env
int qsf_env_init(const char* file);
void qsf_env_exit(void);
