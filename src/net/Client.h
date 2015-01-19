// Copyright (C) 2014 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#pragma once

#include <cstdint>
#include <cassert>
#include <string>
#include <vector>
#include <functional>
#include <system_error>
#include <asio/ip/tcp.hpp>
#include <asio/io_service.hpp>
#include <asio/steady_timer.hpp>
#include "core/Range.h"
#include "Packet.h"
#include "IOBuf.h"

namespace net {

class Client
{
public:
    typedef std::function<void(const std::error_code& ec)> ConnectCallback;
    typedef std::function<void(ByteRange)>  ReadCallback;

public:
    explicit Client(asio::io_service& io_service, 
                    uint32_t heart_beat_sec = DEFAULT_MAX_HEARTBEAT_SEC,
                    uint16_t no_compress_size = DEFAULT_NO_COMPRESSION_SIZE,
                    uint8_t xor_key = DEFAULT_XOR_KEY);
    ~Client();

    Client(const Client&) = delete;
    Client& operator = (const Client&) = delete;

    void Connect(const std::string& host, uint16_t port);
    void Connect(const std::string& host, uint16_t port, ConnectCallback callback);

    void StartRead(ReadCallback callback);
    void Write(ByteRange data);
    void Write(const std::string& str) { Write(ByteRange(StringPiece(str))); }
    void Write(const void* data, size_t size)
    {
        assert(data && size > 0);
        Write(ByteRange(reinterpret_cast<const uint8_t*>(data), size));
    }

    void Stop();

private:
    void ReadHead();
    void HandleReadHead(const std::error_code& ec, size_t bytes);
    void HandleReadBody(const std::error_code& ec, size_t bytes);
    void HandleSend(const std::error_code& ec, 
                    size_t bytes, 
                    std::shared_ptr<IOBuf> buf);
    void HeartBeating();

private:
    asio::ip::tcp::socket    socket_;
    asio::steady_timer       heart_beat_;

    ServerHeader            head_;
    std::vector<uint8_t>    buffer_;
    std::vector<uint8_t>    buffer_more_;
    ReadCallback            on_read_;

    time_t      last_send_time_ = 0;
    uint8_t     xor_key_;

    const uint16_t  no_compress_size_;
    const uint32_t  heart_beat_sec_;
};

typedef std::shared_ptr<Client> ClientPtr;

} // namespace net
