// Copyright (C) 2014-2015 chenqiang@chaoyuehudong.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#include "qsf.h"
#include <signal.h>

int main(int argc, char* argv[])
{
    const char* default_file = "config";
    if (argc >= 2)
    {
        default_file = argv[1];
    }
    signal(SIGILL, qsf_exit);
    signal(SIGTERM, qsf_exit);
    return qsf_start(default_file);
}
