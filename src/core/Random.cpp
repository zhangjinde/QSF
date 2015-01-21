// Copyright (C) 2014 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

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

#include "Random.h"
#include <random>
#include <cassert>
#include "Platform.h"
#include "Logging.h"


static std::default_random_engine* get_tls_rng()
{
    static THREAD_LOCAL std::default_random_engine* rng = nullptr;
    if (rng == nullptr)
    {
        std::random_device gen;
        rng = new std::default_random_engine(gen());
    }
    assert(rng != nullptr);
    return rng;
}

void Random::release()
{
    delete get_tls_rng();
}

void Random::seed(int32_t seed_value)
{
    if (seed_value == 0)
    {
        std::random_device device;
        seed_value = device();
    }
    get_tls_rng()->seed(seed_value);
}

uint32_t Random::rand32()
{
    uint32_t r = (*get_tls_rng())();
    return r;
}

uint32_t Random::rand32(uint32_t max)
{
    if (max == 0)
    {
        return 0;
    }
    return std::uniform_int_distribution<uint32_t>(0, max - 1)(*get_tls_rng());
}

uint32_t Random::rand32(uint32_t min, uint32_t max)
{
    if (min == max)
    {
        return 0;
    }
    return std::uniform_int_distribution<uint32_t>(min, max - 1)(*get_tls_rng());
}

uint64_t Random::rand64(uint64_t max)
{
    if (max == 0)
    {
        return 0;
    }
    return std::uniform_int_distribution<uint64_t>(0, max - 1)(*get_tls_rng());
}

uint64_t Random::rand64(uint64_t min, uint64_t max)
{
    if (min == max)
    {
        return 0;
    }
    return std::uniform_int_distribution<uint64_t>(min, max - 1)(*get_tls_rng());
}

bool Random::oneIn(uint32_t n)
{
    if (n == 0)
    {
        return false;
    }
    return rand32(n) == 0;
}
