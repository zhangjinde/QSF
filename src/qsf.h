// Copyright (C) 2014-2015 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#pragma once


// Start framework
int qsf_start(const char* file);

// Create a ZMQ dealer object
void* qsf_create_dealer(const char* name);
