#include <gtest/gtest.h>
#include <algorithm>
#include <thread>
#include <vector>
#include "core/Random.h"
#include "core/Logging.h"

using namespace std;

TEST(Random, Simple) 
{
    uint32_t prev = 0, seed = 0;
    Random::seed();
    for (int i = 0; i < 1024; ++i) 
    {
        EXPECT_NE(seed = Random::rand32(), prev);
        prev = seed;
    }
}

TEST(Random, rand64)
{
    uint64_t prev = 0, seed = 0;
    Random::seed();
    for (int i = 0; i < 1024; ++i)
    {
        EXPECT_NE(seed = Random::rand64(), prev);
        prev = seed;
    }
}
