// Copyright (C) 2015 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#pragma once

#include <stdio.h>

/*
 * Turn A into a string literal without expanding macro definitions
 * (however, if invoked from a macro, macro arguments are expanded).
 */
#define QSF_STRINGIZE_NX(x) #x

/*
 * Turn A into a string literal after macro-expanding it.
 */
#define QSF_STRINGIZE(A)    QSF_STRINGIZE_NX(A)

// Compiler specific attribute translation
// msvc should come first, so if clang is in msvc mode it gets the right defines
// NOTE: this will only do checking in msvc with versions that support /analyze
#if _MSC_VER
# ifdef _USE_ATTRIBUTES_FOR_SAL
#   undef _USE_ATTRIBUTES_FOR_SAL
# endif
# define _USE_ATTRIBUTES_FOR_SAL 1
# include <sal.h>
# define PRINTF_FORMAT _Printf_format_string_
# define PRINTF_FORMAT_ATTR(format_param, dots_param) /**/
#else
# define PRINTF_FORMAT /**/
# define PRINTF_FORMAT_ATTR(format_param, dots_param) \
  __attribute__((format(printf, format_param, dots_param)))
#endif


#define qsf_log(fmt, ...) \
    qsf_vlog(__FILE__, __LINE__, fmt, __VA_ARGS__)

#define qsf_assert(expr, fmt, ...) \
    do { \
    if (!(expr)){ \
        char format[100]; \
        snprintf(format, sizeof(format), "expr: %s, %s", QSF_STRINGIZE((expr)), fmt); \
        qsf_vlog(__FILE__, __LINE__, format, __VA_ARGS__); \
        qsf_abort(); \
    }}while(0)


void qsf_vlog(const char* file, int line, const char* fmt, ...)
    PRINTF_FORMAT_ATTR(3,4);

int qsf_log_to_file(int enable);

void qsf_abort(void);