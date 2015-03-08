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


Random::Random(uint32_t seed_value)
{
    seed(seed_value);
}

Random::~Random()
{
}

void Random::seed(uint32_t seed_value)
{
    if (seed_value == 0)
    {
        std::random_device device;
        seed_value = device();
    }
    rand_engine_.seed(seed_value);
}


uint32_t Random::rand32(uint32_t max)
{
    if (max == 0)
    {
        return 0;
    }
    return std::uniform_int_distribution<uint32_t>(0, max - 1)(rand_engine_);
}

uint32_t Random::rand32(uint32_t min, uint32_t max)
{
    if (min == max)
    {
        return 0;
    }
    return std::uniform_int_distribution<uint32_t>(min, max - 1)(rand_engine_);
}

uint64_t Random::rand64(uint64_t max)
{
    if (max == 0)
    {
        return 0;
    }
    return std::uniform_int_distribution<uint64_t>(0, max - 1)(rand_engine_);
}

uint64_t Random::rand64(uint64_t min, uint64_t max)
{
    if (min == max)
    {
        return 0;
    }
    return std::uniform_int_distribution<uint64_t>(min, max - 1)(rand_engine_);
}

bool Random::oneIn(uint32_t n)
{
    if (n == 0)
    {
        return false;
    }
    return rand32(n) == 0;
}
