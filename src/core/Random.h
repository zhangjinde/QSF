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

#pragma once

#include <cstdint>


/**
 * A thread-safe PRNG, use a mersenne twister random number generator.
 */
class Random
{
public:

    // Initialize PRNG pointer and seed
    static void seed(int32_t seed = 0);

    // Delete PRNG pointer
    static void release();

    /**
     * Returns a random uint32_t
     */
    static uint32_t rand32();

    /**
     * Returns a random uint32_t in [0, max). If max == 0, returns 0.
     */
    static uint32_t rand32(uint32_t max);

    /**
     * Returns a random uint32_t in [min, max). If min == max, returns 0.
     */
    static uint32_t rand32(uint32_t min, uint32_t max);

    /**
     * Returns a random uint64_t
     */
    static uint64_t rand64()
    {
        return ((uint64_t)rand32() << 32) | rand32();
    }

    /**
     * Returns a random uint64_t in [0, max). If max == 0, returns 0.
     */
    static uint64_t rand64(uint64_t max);

    /**
     * Returns a random uint64_t in [min, max). If min == max, returns 0.
     */
    static uint64_t rand64(uint64_t min, uint64_t max);

    /**
     * Returns true 1/n of the time. If n == 0, always returns false
     */
    static bool oneIn(uint32_t n);
};

