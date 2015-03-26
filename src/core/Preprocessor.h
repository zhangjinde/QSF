/*
 * Copyright 2014 Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// @author: Andrei Alexandrescu

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

/**
 * FB_ANONYMOUS_VARIABLE(str) introduces an identifier starting with
 * str and ending with a number that varies with the line.
 */
#ifndef FB_ANONYMOUS_VARIABLE
#define FB_CONCATENATE_IMPL(s1, s2) s1##s2
#define FB_CONCATENATE(s1, s2) FB_CONCATENATE_IMPL(s1, s2)
#ifdef __COUNTER__
#define FB_ANONYMOUS_VARIABLE(str) FB_CONCATENATE(str, __COUNTER__)
#else
#define FB_ANONYMOUS_VARIABLE(str) FB_CONCATENATE(str, __LINE__)
#endif
#endif

/*
 * Turn A into a string literal without expanding macro definitions
 * (however, if invoked from a macro, macro arguments are expanded).
 */
#define STRINGIZE_NX(x) #x

/*
 * Turn A into a string literal after macro-expanding it.
 */
#define STRINGIZE(A)    STRINGIZE_NX(A)
