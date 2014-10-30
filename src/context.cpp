// Copyright (C) 2014 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#include "context.h"
#include <cassert>
#include "core/logging.h"
#include "qsf.h"


Context::Context(const std::string& id)
    : name_(id), socket_(qsf::CreateDealer(id))
{
}

Context::~Context()
{
}

size_t  Context::Recv(void(*handler)(const std::string&, ByteRange), bool dont_wait)
{
    assert(handler);
    zmq::message_t from;
    int flag = dont_wait ? ZMQ_DONTWAIT : 0;
    if (socket_->recv(&from, flag))
    {
        std::string name;
        name.assign((const char*)from.data(), from.size());
        zmq::message_t msg;
        if (socket_->recv(&msg, flag))
        {
            ByteRange data((const uint8_t*)msg.data(), msg.size());
            handler(name, data);
            return msg.size();
        }
    }
    return 0;
}

void Context::Send(const std::string& name, ByteRange data)
{
    if (!name.empty() && !data.empty())
    {
        socket_->send(name.c_str(), name.length(), ZMQ_SNDMORE);
        socket_->send(data.data(), data.size());
    }
}
