#include <cstdlib>
#include <iostream>
#include <random>
#include <gtest/gtest.h>
#include "core/Benchmark.h"


using namespace std;


int main(int argc, char* argv[])
{
    random_device device;
    srand(device());

    testing::InitGoogleTest(&argc, argv);    

    int r = RUN_ALL_TESTS();

    // run benchmarks
    cout << "\nPATIENCE, BENCHMARKS IN PROGRESS." << endl;
    runBenchmarks();
    cout << "MEASUREMENTS DONE." << endl;

    return r;
}
