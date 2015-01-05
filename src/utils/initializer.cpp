// Copyright (C) 2014 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#include "initializer.h"
#include <time.h>
#include <stdlib.h>
#include <stdint.h>
#include "core/logging.h"

#ifdef _WIN32
#include <Objbase.h>
#endif 

Initializer::Initializer()
{
    init();
}

Initializer::~Initializer()
{
    deinit();
}

void Initializer::init()
{
    srand(static_cast<uint32_t>(time(NULL)));
#ifdef _WIN32
    CHECK(CoInitialize(NULL) == S_OK);
#endif
}

void Initializer::deinit()
{
#ifdef _WIN32
    CoUninitialize();
#endif
}