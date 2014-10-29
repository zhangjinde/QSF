// Copyright (C) 2014 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#pragma once

#include <memory>
#include <functional>
#include <zmq.hpp>
#include "core/range.h"


class Context
{
public:
    explicit Context(const std::string& name);
    ~Context();

    const std::string& Name() { return name_; }

    // recieve message
    size_t  Recv(void (*handler)(const std::string&, ByteRange), 
                 bool dont_wait = false);

    // send message to other object
    void    Send(const std::string& name, ByteRange json);

private:
    // zmq socket object
    std::unique_ptr<zmq::socket_t>  socket_;

    // identity of this object, should be unique in current process
    std::string  name_;
};
