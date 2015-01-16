// Copyright (C) 2014 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#pragma once

#include <cassert>
#include <memory>
#include <functional>
#include <zmq.hpp>
#include "core/Range.h"


class Context
{
public:
    typedef std::function<void(StringPiece, StringPiece)> HandlerType;

public:
    explicit Context(std::unique_ptr<zmq::socket_t> socket,
        const std::string& name)
        : socket_(std::move(socket)), name_(name)
    {
    }

    ~Context()
    {
    }

    // Unique name of this context
    const std::string& name() { return name_; }

    // Recieve message
    size_t  recv(const HandlerType& handler, bool dont_wait = false)
    {
        assert(handler);
        zmq::message_t from;
        int flag = dont_wait ? ZMQ_DONTWAIT : 0;
        if (socket_->recv(&from, flag))
        {
            assert(from.size() <= 16);
            zmq::message_t msg;
            if (socket_->recv(&msg, flag))
            {
                StringPiece name((const char*)from.data(), from.size());
                StringPiece data((const char*)msg.data(), msg.size());
                handler(name, data);
                return msg.size();
            }
        }
        return 0;
    }

    // Send message to other context
    void    send(StringPiece name, StringPiece data)
    {
        if (!name.empty() && !data.empty())
        {
            socket_->send(name.data(), name.size(), ZMQ_SNDMORE);
            socket_->send(data.data(), data.size());
        }
    }

private:
    // 0mq socket object
    std::unique_ptr<zmq::socket_t>  socket_;

    // Identity of this object, should be unique in current process
    std::string  name_;
};
