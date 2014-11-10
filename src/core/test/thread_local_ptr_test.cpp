#include <gtest/gtest.h>
#include <vector>
#include <thread>
#include <unordered_set>
#include "core/thread_local_ptr.h"

using namespace std;

TEST(ThreadLocalPtr, Construct)
{
    auto ptr = new int(1024);
    ThreadLocalPtr<int> p(ptr);
    EXPECT_EQ(p.get(), ptr);
    EXPECT_FALSE(!p);
    EXPECT_EQ(*p, 1024);

    p.reset(nullptr);
    EXPECT_EQ(p.get(), nullptr);
    EXPECT_TRUE(!p);
}

TEST(ThreadLocalPtr, Swap)
{
    ThreadLocalPtr<int> p1(new int(1234));
    ThreadLocalPtr<int> p2(new int(5678));
    EXPECT_EQ(*p1, 1234);
    EXPECT_EQ(*p2, 5678);

    std::swap(p1, p2);
    EXPECT_EQ(*p2, 1234);
    EXPECT_EQ(*p1, 5678);
}

TEST(ThreadLocalPtr, MultiThreaded)
{
    const int n = 1024;
    std::vector<int*> ptrs(n);
    std::vector<std::thread> threads;
    bool aborted = false;
    bool ready = false;
    for (int i = 1; i <= n; ++i)
    {
        threads.push_back(std::thread([i, &ptrs, &aborted, &ready, &n]
        {
            ThreadLocalPtr<int> p(new int(i));
            EXPECT_NE(p.get(), nullptr);
            ptrs[i-1] = p.get();
            if (i == n)
            {
                ready = true;
            }
            while (!aborted)
            {
                std::this_thread::yield();
            }
        }));
    }
    while (!ready)
    {
        std::this_thread::yield();
    }
    std::unordered_set<int*> set;
    for (auto p : ptrs)
    {
        EXPECT_TRUE(set.count(p) == 0);
        set.insert(p);
    }
    aborted = true;
    for (auto& t : threads)
    {
        t.join();
    }
}
