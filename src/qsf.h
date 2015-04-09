// Copyright (C) 2014-2015 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#pragma once

// start qsf framework with a config file
int qsf_start(const char* file);

// create a zmq dealer object connected to router
void* qsf_create_dealer(const char* name);

// global zmq context object
void* qsf_zmq_context(void);