// Copyright (C) 2014-2015 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#pragma once

#include <stdlib.h>

// http://www.canonware.com/download/jemalloc/jemalloc-latest/doc/jemalloc.html
#ifdef USE_JEMALLOC
# include <jemalloc/jemalloc.h>
#endif

// `malloc` functions are defined as weak symbols in glibc.
#define qsf_malloc     malloc
#define qsf_free       free
#define qsf_realloc    realloc
