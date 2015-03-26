// Copyright (C) 2014 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#include <cstdio>
#include <csignal>
#include <exception>
#include <typeinfo>
#include "qsf.h"

using namespace std;

int main(int argc, char* argv[])
{
    const char* default_file = "config";
    if (argc >= 2)
    {
        default_file = argv[1];
    }
    
    return qsf::Start(default_file);
}
