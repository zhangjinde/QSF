// Copyright (C) 2014 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#pragma once

#include <cstdint>
#include <ctime>
#include <cassert>
#include <string>
#include <memory>
#include <system_error>
#include <unordered_set>
#include <unordered_map>
#include <asio/io_service.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/steady_timer.hpp>
#include "Packet.h"
#include "IOBuf.h"

namespace net {

typedef std::function<void(int, uint32_t, ByteRange)>   ReadCallback;

class Gate
{
    struct Session;
    typedef std::shared_ptr<Session> SessionPtr;

public:
    explicit Gate(asio::io_service& io_service, 
                  uint32_t max_connections = DEFAULT_MAX_CONNECTIONS,
                  uint32_t heart_beat_sec = DEFAULT_MAX_HEARTBEAT_SEC,
                  uint32_t heart_beat_check_sec = DEFAULT_HEARTBEAT_CHECK_SEC,
                  uint16_t max_no_compress_size = DEFAULT_NO_COMPRESSION_SIZE,
                  uint8_t xor_key = DEFAULT_XOR_KEY);
    ~Gate();

    Gate(const Gate&) = delete;
    Gate& operator = (const Gate&) = delete;

    void Start(const std::string& host, uint16_t port, ReadCallback callback);

    void Stop();

    bool Send(uint32_t serial, ByteRange data);
    bool Send(uint32_t serial, const std::string& str)
    {
        return Send(serial, ByteRange(StringPiece(str)));
    }
    bool Send(uint32_t serial, const void* buf, size_t size)
    {
        assert(buf && size > 0);
        return Send(serial, ByteRange(reinterpret_cast<const uint8_t*>(buf), size));
    }
    void SendAll(const void* buf, size_t size);

    bool Kick(uint32_t serial);
    void KickAll();

    uint32_t GetSessionCount() const { return sessions_.size(); }

    void DenyAddress(const std::string& address);
    void AllowAddress(const std::string& address);

private:
    uint32_t NextSerial();
    SessionPtr GetSession(uint32_t serial);
    void Kick(SessionPtr session);
    void CheckHeartBeat();
    void StartAccept();
    void SessionStartRead(SessionPtr session);
    bool SessionWritePacket(SessionPtr session, ByteRange data);
    void SessionWriteFrame(SessionPtr session, ByteRange frame, PacketType type);

    void HandleAccept(const std::error_code& err, SessionPtr ptr);
    void HandleReadHead(uint32_t serial, const std::error_code& ec, size_t bytes);
    void HandleReadBody(uint32_t serial, const std::error_code& ec, size_t bytes);
    void HandleWrite(uint32_t serial, const std::error_code& ec, size_t bytes, 
                     std::shared_ptr<IOBuf> buf);

private:
    asio::io_service&        io_service_;
    asio::ip::tcp::acceptor  acceptor_;
    asio::steady_timer       drop_timer_;

    ReadCallback    on_read_;

    
    uint32_t current_serial_ = 1000;

    std::unordered_map<uint32_t, SessionPtr> sessions_;

    std::unordered_set<std::string>  black_list_;

    uint8_t     xor_key_;

    const uint16_t max_no_compress_size_;
    const uint32_t heart_beat_sec_;
    const uint32_t heart_beat_check_sec_;
    const uint32_t max_connections_;
};

typedef std::shared_ptr<Gate>   GatePtr;

} // namespace net
