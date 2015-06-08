// Copyright (C) 2014-2015 chenqiang@chaoyuehudong.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#pragma once

#if defined(__GNUC__) && __GNUC__ >= 4
#define LIKELY(x)   (__builtin_expect((x), 1))
#define UNLIKELY(x) (__builtin_expect((x), 0))
#else
#define LIKELY(x)   (x)
#define UNLIKELY(x) (x)
#endif

/*
 * Compiler specific attribute translation
 * msvc should come first, so if clang is in msvc mode it gets the right defines
 * NOTE: this will only do checking in msvc with versions that support /analyze
 */
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

/*
 * detection for 64 bit
 */
#if defined(__x86_64__) || defined(_M_X64)
# define QSF_X64  1
#else
# define QSF_X64  0
#endif

/* 
 * platform specific TLS support
 * gcc implements __thread
 * msvc implements __declspec(thread)
 * the semantics are the same (but remember __thread is broken on apple)
 */
#if defined(_MSC_VER)
# define QSF_TLS    __declspec(thread)
#elif defined(__GNUC__) || defined(__clang__)
# define QSF_TLS    __thread
#else
# error cannot define platform specific thread local storage
#endif

#ifndef MAX_PATH
#define MAX_PATH    260
#endif
