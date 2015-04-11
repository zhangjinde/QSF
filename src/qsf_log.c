// Copyright (C) 2015 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#include "qsf_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <time.h>

#ifdef _WIN32
#include <Windows.h>
#endif

#if defined(_MSC_VER)
# define QSF_TLS    __declspec(thread)
#elif defined(__GNUC__) || defined(__clang__)
# define QSF_TLS    __thread
#else
# error cannot define platform specific thread local storage
#endif

#define LOG_BUFSIZE     8196

static const char* qsf_file_name = "qsf.log";
static volatile int qsf_enable_log_to_file = 1;

// global thread local log buffer
QSF_TLS char global_log_buffer[LOG_BUFSIZE];

void write_log_to_file(const char* msg, int size)
{
    FILE* fp = fopen(qsf_file_name, "a");
    if (fp)
    {
        time_t now = time(NULL);
        char timestamp[40];
        size_t ch = strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S ", localtime(&now));
        assert(ch > 0);
        fwrite(timestamp, 1, ch, fp);
        fwrite(msg, 1, size, fp);
        fclose(fp);
    }
}

void qsf_abort(void)
{
    abort();
}

int qsf_log_to_file(int enable)
{
#ifdef _WIN32
    return (int)InterlockedExchange((LONG*)&qsf_enable_log_to_file, enable);
#else
    return (int)__sync_lock_test_and_set(&qsf_enable_log_to_file, enable);
#endif
}

void qsf_vlog(const char* file, int line, const char* fmt, ...)
{
    int ch = snprintf(global_log_buffer, LOG_BUFSIZE, "%s[%d]: ", file, line);
    assert(ch > 0);
    va_list ap;
    va_start(ap, fmt);
    int bytes = vsnprintf(global_log_buffer + ch, LOG_BUFSIZE - ch, fmt, ap);
    va_end(ap);
    if (bytes > 0)
    {
        int size = ch + bytes;
        assert(size + 2 < LOG_BUFSIZE);
        global_log_buffer[size] = '\n';
        global_log_buffer[size + 1] = '\0';
        fprintf(stderr, global_log_buffer);
        if (qsf_enable_log_to_file)
        {
            write_log_to_file(global_log_buffer, size);
        }
#ifdef _WIN32
        OutputDebugStringA(global_log_buffer);
#endif
    }
}