// Copyright (C) 2014-2015 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#include "qsf.h"

int main(int argc, char* argv[])
{
    const char* default_file = "config";
    if (argc >= 2)
    {
        default_file = argv[1];
    }
    
    return qsf_start(default_file);
}
