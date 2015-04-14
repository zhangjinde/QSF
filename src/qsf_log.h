// Copyright (C) 2015 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#pragma once

#include <stdio.h>

 
#if defined(__GNUC__) && __GNUC__ >= 4
#define LIKELY(x)   (__builtin_expect((x), 1))
#define UNLIKELY(x) (__builtin_expect((x), 0))
#else
#define LIKELY(x)   (x)
#define UNLIKELY(x) (x)
#endif

// Compiler specific attribute translation
// msvc should come first, so if clang is in msvc mode it gets the right defines
// NOTE: this will only do checking in msvc with versions that support /analyze
#if _MSC_VER
# ifdef _USE_ATTRIBUTES_FOR_SAL
#   undef _USE_ATTRIBUTES_FOR_SAL
# endif
# define _USE_ATTRIBUTES_FOR_SAL 1
# include <sal.h>
# define PRINTF_FORMAT  _Printf_format_string_
# define PRINTF_FORMAT_ATTR(format_param, dots_param) /**/
#else
# define PRINTF_FORMAT /**/
# define PRINTF_FORMAT_ATTR(format_param, dots_param) \
    __attribute__((format(printf, format_param, dots_param)))
#endif

#define qsf_log(fmt, ...) \
    qsf_vlog(__FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define qsf_assert(expr, fmt, ...) \
    do { \
    if (UNLIKELY(!(expr))){ \
        qsf_vlog(__FILE__, __LINE__, fmt, ##__VA_ARGS__); \
        qsf_abort(); \
    }}while(0)

// abort current process
void qsf_abort(void);

void qsf_vlog(const char* file, int line, const char* fmt, ...)
    PRINTF_FORMAT_ATTR(3, 4);

// enable/disable `qsf_vlog` write to file
int qsf_log_to_file(int enable);
