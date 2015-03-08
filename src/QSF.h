// Copyright (C) 2014 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#pragma once

#include <memory>
#include <string>
#include <zmq.hpp>


enum
{
    // Max size for identity of each dealer
    MAX_NAME_SIZE = 16,
};

namespace qsf {

// Create a dealer object, this function is thread-safe
std::unique_ptr<zmq::socket_t> CreateDealer(const std::string& id);

// Create a named service
bool CreateService(const std::string& type, 
                   const std::string& name, 
                   const std::string& args);

// Run services and message dispatching
int  Start(const char* config);

// Stop all service
void Exit();


};
