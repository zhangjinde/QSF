// Copyright (C) 2014-2015 chenqiang@chaoyuehudong.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#pragma once

// identify the current platform
#if defined(__linux__)
#define PLATFORM_LINUX    1
#define PLATFORM_STRING   "linux"
#elif defined(_WIN32)
#define PLATFORM_WINDOWS  1
#define PLATFORM_STRING   "windows"
#elif defined(__APPLE__) && defined(__MACH__)
#define PLATFORM_MACOSX   1
#define PLATFORM_STRING   "macosx"
#else
#error "unsupported platform"
#endif

// detection for 64 bit
#if defined(__x86_64__) || defined(_M_X64)
# define ARCH_X64  1
#else
# define ARCH_X64  0
#endif

#if defined(__GNUC__) && __GNUC__ >= 4
#define LIKELY(x)   (__builtin_expect((x), 1))
#define UNLIKELY(x) (__builtin_expect((x), 0))
#else
#define LIKELY(x)   (x)
#define UNLIKELY(x) (x)
#endif


#if defined(_MSC_VER)
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

#if defined(_MSC_VER) && (_MSC_VER < 1900)
#define snprintf    _snprintf
#endif

#ifndef MAX_PATH
#define MAX_PATH    260
#endif
