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

#include <array>
#include <initializer_list>
#include <random>
#include <vector>
#include <gtest/gtest.h>
#include "Benchmark.h"
#include "core/Logging.h"
#include "core/Range.h"
#include "core/Varint.h"


using namespace std;


void testVarint(uint64_t val, std::initializer_list<uint8_t> bytes) 
{
    size_t n = bytes.size();
    ByteRange expected(&*bytes.begin(), n);

    {
        uint8_t buf[kMaxVarintLength64];
        EXPECT_EQ(expected.size(), encodeVarint(val, buf));
        EXPECT_TRUE(ByteRange(buf, expected.size()) == expected);
    }

  {
      ByteRange r = expected;
      uint64_t decoded = decodeVarint(r);
      EXPECT_TRUE(r.empty());
      EXPECT_EQ(val, decoded);
  }

    if (n < kMaxVarintLength64) 
    {
        // Try from a full buffer too, different code path
        uint8_t buf[kMaxVarintLength64];
        memcpy(buf, &*bytes.begin(), n);

        uint8_t fills[] = { 0, 0x7f, 0x80, 0xff };

        for (uint8_t fill : fills) 
        {
            memset(buf + n, fill, kMaxVarintLength64 - n);
            ByteRange r(buf, kMaxVarintLength64);
            uint64_t decoded = decodeVarint(r);
            EXPECT_EQ(val, decoded);
            EXPECT_EQ(kMaxVarintLength64 - n, r.size());
        }
    }
}

TEST(Varint, Simple) 
{
    testVarint(0, { 0 });
    testVarint(1, { 1 });
    testVarint(127, { 127 });
    testVarint(128, { 0x80, 0x01 });
    testVarint(300, { 0xac, 0x02 });
    testVarint(16383, { 0xff, 0x7f });
    testVarint(16384, { 0x80, 0x80, 0x01 });

    testVarint(static_cast<uint32_t>(-1),
    { 0xff, 0xff, 0xff, 0xff, 0x0f });
    testVarint(static_cast<uint64_t>(-1),
    { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x01 });
}

TEST(ZigZag, Simple) 
{
    EXPECT_EQ(0, encodeZigZag(0));
    EXPECT_EQ(1, encodeZigZag(-1));
    EXPECT_EQ(2, encodeZigZag(1));
    EXPECT_EQ(3, encodeZigZag(-2));
    EXPECT_EQ(4, encodeZigZag(2));

    EXPECT_EQ(0, decodeZigZag(0));
    EXPECT_EQ(-1, decodeZigZag(1));
    EXPECT_EQ(1, decodeZigZag(2));
    EXPECT_EQ(-2, decodeZigZag(3));
    EXPECT_EQ(2, decodeZigZag(4));
}

namespace {

const size_t kNumValues = 1000;
std::vector<uint64_t> gValues;
std::vector<uint64_t> gDecodedValues;
std::vector<uint8_t> gEncoded;

BENCHMARK_INITIALIZER(generateRandomValues)
{
    std::mt19937 rng;

    // Approximation of power law
    std::uniform_int_distribution<int> numBytes(1, 8);
    std::uniform_int_distribution<int> byte(0, 255);

    gValues.resize(kNumValues);
    gDecodedValues.resize(kNumValues);
    gEncoded.resize(kNumValues * kMaxVarintLength64);
    for (size_t i = 0; i < kNumValues; ++i)
    {
        int n = numBytes(rng);
        uint64_t val = 0;
        for (size_t j = 0; j < n; ++j)
        {
            val = (val << 8) + byte(rng);
        }
        gValues[i] = val;
    }
}

} // anonymouse namespace


BENCHMARK(VarintEncoding, iters)
{
    uint8_t* start = &(*gEncoded.begin());
    uint8_t* p = start;
    bool empty = (iters == 0);
    while (iters--)
    {
        p = start;
        for (auto& v : gValues)
        {
            p += encodeVarint(v, p);
        }
    }

    //gEncoded.erase(gEncoded.begin() + (p - start), gEncoded.end());
}

BENCHMARK(VarintDecoding, iters)
{
    while (iters--)
    {
        size_t i = 0;
        ByteRange range(&(*gEncoded.begin()), gEncoded.size());
        while (!range.empty() && i < kNumValues)
        {
            gDecodedValues[i++] = decodeVarint(range);
        }
    }
}
