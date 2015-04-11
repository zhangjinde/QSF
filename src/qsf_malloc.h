// Copyright (C) 2014-2015 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#pragma once

#ifdef USE_JEMALLOC
# include <jemalloc/jemalloc.h>
# define qsf_malloc     je_malloc
# define qsf_free       je_free
# define qsf_realloc    je_realloc
#else 
# ifdef __APPLE__
#  include <malloc/malloc.h>
# else
#  include <malloc.h>
# endif
# define qsf_malloc     malloc
# define qsf_free       free
# define qsf_realloc    realloc
#endif 
