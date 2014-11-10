// Copyright (C) 2014 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <zmq.hpp>
#include "core/strings.h"
#include "context.h"
#include "service.h"


enum
{
    // max size for each single inproc message, 32M
    MAX_MSG_SIZE = (1024 * 1024 * 32),

    // max size for identity of each dealer
    MAX_NAME_SIZE = 16,
};

namespace qsf {

// create a dealer object, thread-safe
std::unique_ptr<zmq::socket_t> createDealer(const std::string& id);

// create a context
bool createService(const std::string& type, 
                   const std::string& name, 
                   const std::string& args);

// run service and message dispatching
int  start(const char* config);

// stop all service
void stop();

};

