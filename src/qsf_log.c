// Copyright (C) 2014-2015 chenqiang@chaoyuehudong.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#include "qsf_log.h"
#include "qsf.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <time.h>

#ifdef _WIN32
#include <Windows.h>
#endif


static const char* qsf_file_name = "qsf.log";
static volatile int qsf_enable_log_to_file = 0;


static void write_log_to_file(const char* msg, int size)
{
    FILE* fp = fopen(qsf_file_name, "a");
    if (fp)
    {
        time_t now = time(NULL);
        struct tm date = *localtime(&now);
        char timestamp[40];
        size_t bytes = strftime(timestamp, 40, "%Y-%m-%d %H:%M:%S ", &date);
        assert(bytes > 0);
        fwrite(timestamp, 1, bytes, fp);
        fwrite(msg, 1, size, fp);
        fclose(fp);
    }
}

void qsf_abort(const char* msg)
{
#ifdef _WIN32
    //  Raise STATUS_FATAL_APP_EXIT.
    ULONG_PTR extra_info[1];
    extra_info[0] = (ULONG_PTR)msg;
    RaiseException(0x40000015, EXCEPTION_NONCONTINUABLE, 1, extra_info);
#else
    (void)msg;
    abort();
#endif
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
    char buffer[2048];
    char* bufptr = buffer;
    int left_bytes = sizeof(buffer);
    int bytes = snprintf(bufptr, left_bytes, "%s[%d]: ", file, line);
    assert(bytes > 0);
    bufptr += bytes;
    left_bytes -= bytes;

    va_list ap;
    va_start(ap, fmt);
    bytes = vsnprintf(bufptr, left_bytes, fmt, ap);
    va_end(ap);
    bufptr += bytes;
    left_bytes -= bytes;
    if (bytes <= 0)
    {
        return;
    }

    if (left_bytes >= 2)
    {
        *(bufptr++) = '\n';
        *bufptr = '\0';
        fprintf(stderr, "%s", buffer);
        if (qsf_enable_log_to_file)
        {
            write_log_to_file(buffer, (int)(bufptr - buffer));
        }
#ifdef _WIN32
        OutputDebugStringA(buffer);
#endif
    }
}
