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
    typedef std::function<void(StringPiece, StringPiece)> HandlerType;

public:
    explicit Context(const std::string& name);
    ~Context();

    // Unique name of this context
    const std::string& name() { return name_; }

    // Recieve message
    size_t  recv(const HandlerType& handler, bool dont_wait = false);

    // Send message to other context
    void    send(StringPiece name, StringPiece data);

    zmq::socket_t& socket() { return *socket_; }

private:
    // 0mq socket object
    std::unique_ptr<zmq::socket_t>  socket_;

    // Identity of this object, should be unique in current process
    std::string  name_;
};
