// Copyright (C) 2014 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#pragma once

#if defined(__x86_64__) || defined(_M_X64)
# define PLATFORM_X64   1
#else
# define PLATFORM_X64   0
#endif

// Portable version check
#ifndef __GNUC_PREREQ
# if defined __GNUC__ && defined __GNUC_MINOR__
#  define __GNUC_PREREQ(maj, min) ((__GNUC__ << 16) + __GNUC_MINOR__ >= \
                                   ((maj) << 16) + (min))
# else
#  define __GNUC_PREREQ(maj, min) 0
# endif
#endif

#if defined(__GNUC__) && __GNUC__ >= 4
# define LIKELY(x)   (__builtin_expect((x), 1))
# define UNLIKELY(x) (__builtin_expect((x), 0))
#else
# define LIKELY(x)   (x)
# define UNLIKELY(x) (x)
#endif

#ifdef _MSC_VER
# define ALIGN(x)   __declspec(align(x))
#else
# define ALIGN(x)   __attribute__((aligned(x)))
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
# define PRINTF_FORMAT _Printf_format_string_
# define PRINTF_FORMAT_ATTR(format_param, dots_param) /**/
#else
# define PRINTF_FORMAT /**/
# define PRINTF_FORMAT_ATTR(format_param, dots_param) \
  __attribute__((format(printf, format_param, dots_param)))
#endif

#ifdef _MSC_VER
# define API_EXPORT     __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
# define API_EXPORT     __attribute__ ((visibility("default")))
#endif

// MSVC specific defines, mainly for posix compatibility
#ifdef _MSC_VER
#if !defined(_SSIZE_T_) && !defined(_SSIZE_T_DEFINED)
#include <basetsd.h>
typedef SSIZE_T ssize_t;
# define _SSIZE_T_
# define _SSIZE_T_DEFINED
#endif

#define snprintf   _snprintf
#define strerror_r(err,buf,len) strerror_s(buf,len,err)
#define __PRETTY_FUNCTION__     __FUNCSIG__
#define alignof                 __alignof

#if defined(__cplusplus) && _MSC_VER <= 1800
# define noexcept       _NOEXCEPT
#endif

#ifndef __cplusplus 
# define inline __inline
#endif

#endif
