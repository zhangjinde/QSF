// Copyright (C) 2014-2015 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#pragma once

#include <stdlib.h>

#ifdef USE_JEMALLOC
# define qsf_malloc     je_malloc
# define qsf_free       je_free
#else
# define qsf_malloc      malloc
# define qsf_free        free
#endif
