// Copyright (C) 2015 chenqiang@chaoyuehudong.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#pragma once

#include <stdio.h>
#include "qsf.h"

#define qsf_log(fmt, ...) \
    qsf_vlog(__FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define qsf_assert(expr, fmt, ...) \
    do { \
    if (UNLIKELY(!(expr))){ \
        qsf_vlog(__FILE__, __LINE__, fmt, ##__VA_ARGS__); \
        qsf_abort(#expr); \
    }}while(0)

// abort current process
void qsf_abort(const char* msg);

void qsf_vlog(const char* file, int line, const char* fmt, ...)
    PRINTF_FORMAT_ATTR(3, 4);

// enable/disable `qsf_vlog` write to file
int qsf_log_to_file(int enable);
