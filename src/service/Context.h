// Copyright (C) 2014-2015 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#pragma once

#include <cassert>
#include <memory>
#include <string>
#include <functional>
#include <zmq.hpp>
#include "core/Range.h"


class Context
{
public:
    typedef std::function<void(StringPiece, StringPiece)> HandlerType;

public:
    explicit Context(const std::string& name);
    ~Context();

    // Unique name of this context
    const std::string& Name() { return name_; }

    // Recieve message
    size_t  Recv(const HandlerType& handler, bool dont_wait = false);

    // Send message to other context
    void    Send(StringPiece name, StringPiece data);

private:
    // zeromq socket object
    std::unique_ptr<zmq::socket_t>  socket_;

    // Identity of this object, should be unique in current process
    std::string  name_;
};
