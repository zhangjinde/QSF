// Copyright (C) 2014 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#include "context.h"
#include <cassert>
#include "core/logging.h"
#include "qsf.h"


Context::Context(const std::string& id)
    : name_(id), socket_(qsf::createDealer(id))
{
}

Context::~Context()
{
}

size_t  Context::recv(const HandlerType& handler, bool dont_wait)
{
    assert(handler);
    zmq::message_t from;
    int flag = dont_wait ? ZMQ_DONTWAIT : 0;
    if (socket_->recv(&from, flag))
    {
        assert(from.size() <= MAX_NAME_SIZE);
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

void Context::send(StringPiece name, StringPiece data)
{
    if (!name.empty() && !data.empty())
    {
        socket_->send(name.data(), name.size(), ZMQ_SNDMORE);
        socket_->send(data.data(), data.size());
    }
}
