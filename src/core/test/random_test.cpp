#include <gtest/gtest.h>
#include <algorithm>
#include <thread>
#include <vector>
#include "core/random.h"
#include "core/logging.h"

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

TEST(Random, randDouble)
{
    double prev = 0, seed = 0;
    Random::seed();
    for (int i = 0; i < 1024; ++i)
    {
        EXPECT_NE(seed = Random::randDouble01(), prev);
        prev = seed;
    }
}

TEST(Random, MultiThreaded) 
{
    const int n = 1024;
    std::vector<uint32_t> seeds(n);
    std::vector<std::thread> threads;
    for (int i = 0; i < n; ++i) 
    {
        threads.push_back(std::thread([i, &seeds] 
        {
            Random::seed();
            seeds[i] = Random::rand32();
            Random::release();
        }));
    }
    for (auto& t : threads) 
    {
        t.join();
    }
    std::sort(seeds.begin(), seeds.end());
    for (int i = 0; i < n - 1; ++i) 
    {
        EXPECT_LT(seeds[i], seeds[i + 1]);
    }
}
