// Copyright (C) 2014-2015 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#include "Context.h"
#include "qsf.h"

Context::Context(const std::string& name)
    : socket_(std::move(qsf::CreateDealer(name))), name_(name)
{
}

Context::~Context()
{
}

// Recieve message
size_t  Context::Recv(const HandlerType& handler, bool dont_wait)
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

// Send message to other context
void    Context::Send(StringPiece name, StringPiece data)
{
    if (!name.empty() && !data.empty())
    {
        socket_->send(name.data(), name.size(), ZMQ_SNDMORE);
        socket_->send(data.data(), data.size());
    }
}
