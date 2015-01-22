// Copyright (C) 2014 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#include "Gate.h"
#include <ctime>
#include <functional>
#include <system_error>
#include <asio.hpp>
#include "core/Logging.h"
#include "core/Strings.h"
#include "core/Checksum.h"
#include "Compression.h"

using namespace std::placeholders;

namespace net {

enum
{
    DEFAULT_DEAD_CONN_SIZE = 8,
    DEFAULT_SESSION_RECV_BUF = 64,
};

// Connection session object
struct Gate::Session
{
    uint64_t        serial_ = 0;
    bool            closed_ = false;
    time_t          last_recv_time_ = 0;
    ClientHeader    head_;

    std::vector<uint8_t>    recv_buf_;
    std::string             remote_addr_;
    asio::ip::tcp::socket   socket_;

    Session(asio::io_service& io_service, uint64_t serial)
        : socket_(io_service), serial_(serial)
    {
        recv_buf_.reserve(DEFAULT_SESSION_RECV_BUF);
        memset(&head_, 0, sizeof(head_));
    }
};

inline void XORBuffer(void* data, size_t size, uint8_t key)
{
    uint8_t* p = reinterpret_cast<uint8_t*>(data);
    for (size_t i = 0; i < size; i++)
    {
        *(p + i) = *(p + i) ^ key;
    }
}

#define MAKE_UINT64(a, b) (((static_cast<uint64_t>(a) << 32) + (b)))

//////////////////////////////////////////////////////////////////////////

Gate::Gate(asio::io_service& io_service, 
           uint32_t serial_prefix,
           uint32_t max_connections, 
           uint32_t heart_beat_sec,
           uint32_t heart_beat_check_sec,
           uint16_t max_no_compress_size,
           uint8_t xor_key
           )
    : io_service_(io_service), 
      acceptor_(io_service), 
      drop_timer_(io_service),
      max_connections_(max_connections),
      heart_beat_sec_(heart_beat_sec),
      heart_beat_check_sec_(heart_beat_check_sec),
      max_no_compress_size_(max_no_compress_size),
      xor_key_(xor_key),
      serial_prefix_(serial_prefix)
{
}

Gate::~Gate()
{
    Stop();
}

void Gate::Start(const std::string& host, uint16_t port, ReadCallback callback)
{
    using namespace asio::ip;
    assert(callback);
    on_read_ = callback;

    tcp::endpoint endpoint(address::from_string(host), port);
    acceptor_.open(endpoint.protocol());
    acceptor_.set_option(tcp::acceptor::reuse_address(true));
    acceptor_.bind(endpoint);
    acceptor_.listen();

    drop_timer_.expires_from_now(std::chrono::seconds(heart_beat_check_sec_));
    drop_timer_.async_wait(std::bind(&Gate::CheckHeartBeat, this));

    StartAccept();
}

void Gate::Stop()
{
    drop_timer_.cancel();
    acceptor_.close();
    black_list_.clear();
    sessions_.clear();
}

Gate::SessionPtr Gate::GetSession(uint64_t serial)
{
    auto iter = sessions_.find(serial);
    if (iter != sessions_.end())
    {
        return iter->second;
    }
    return SessionPtr();
}

void Gate::Kick(SessionPtr session)
{
    if (!session->closed_ && session->socket_.is_open())
    {
        session->socket_.shutdown(asio::socket_base::shutdown_both);
        session->socket_.close();
        session->closed_ = true;
    }
}

bool Gate::Kick(uint64_t serial)
{
    auto session = GetSession(serial);
    if (session)
    {
        Kick(session);
        return true;
    }
    return false;
}

void Gate::KickAll()
{
    sessions_.clear();
}

bool Gate::Send(uint64_t serial, ByteRange data)
{
    auto session = GetSession(serial);
    if (session)
    {
        return SessionWritePacket(session, data);
    }
    return false;
}

void Gate::SendAll(const void* buf, size_t size)
{
    assert(buf && size > 0);
    for (auto& item : sessions_)
    {
        auto& session = item.second;
        ByteRange data(reinterpret_cast<const uint8_t*>(buf), size);
        SessionWritePacket(session, data);
    }
}

uint64_t Gate::NextSerial()
{
    uint64_t serial = MAKE_UINT64(serial_prefix_, current_serial_);
    while (sessions_.count(serial))
    {
        serial = MAKE_UINT64(serial_prefix_, current_serial_++);
    }
    return serial;
}

void Gate::StartAccept()
{
    auto serial = NextSerial();
    SessionPtr session = std::make_shared<Session>(io_service_, serial);
    acceptor_.async_accept(session->socket_, std::bind(&Gate::HandleAccept,
        this, _1, session));
}

void Gate::HandleAccept(const std::error_code& err, SessionPtr session)
{
    auto serial = session->serial_;
    if (err)
    {
        on_read_(err.value(), serial, StringPiece(err.message()));
        return ;
    }
    if (sessions_.size() < max_connections_)
    {
        auto endpoint = session->socket_.remote_endpoint();
        session->remote_addr_ = endpoint.address().to_string();
        const auto& address = session->remote_addr_;
        if (!black_list_.count(address))
        {
            sessions_[serial] = session;
            SessionStartRead(session);
        }
        else
        {
            auto errmsg = stringPrintf("address [%s] was in black list.", address.c_str());
            on_read_(ERR_ADDRRESS_BANNED, serial, StringPiece(errmsg));
        }
    }
    else
    {
        auto errmsg = stringPrintf("too many connections now, current is: %u", 
            (uint32_t)sessions_.size());
        on_read_(ERR_MAX_CONN_LIMIT, serial, StringPiece(errmsg));
    }
    if (acceptor_.is_open())
    {
        StartAccept();
    }
}

void Gate::DenyAddress(const std::string& address)
{
    black_list_.insert(address);
}

void Gate::AllowAddress(const std::string& address)
{
    black_list_.erase(address);
}

std::string Gate::GetSessionAddress(uint64_t serial)
{
    auto session = GetSession(serial);
    if (session)
    {
        return session->remote_addr_;
    }
    return "";
}

void Gate::CheckHeartBeat()
{
    std::vector<uint64_t> dead_connections;
    dead_connections.reserve(DEFAULT_DEAD_CONN_SIZE);

    // garbage collecting
    for (auto& item : sessions_)
    {
        auto& session = item.second;
        if (session->closed_)
        {
            dead_connections.emplace_back(item.first);
        }
    }
    for (auto serial : dead_connections)
    {
        sessions_.erase(serial);
    }

    // heart-beat checking
    time_t now = time(NULL);
    for (auto& item : sessions_)
    {
        auto& session = item.second;
        auto elapsed = now - session->last_recv_time_;
        if (elapsed >= heart_beat_sec_)
        {
            Kick(session);
            on_read_(ERR_HEARTBEAT_TIMEOUT, session->serial_, StringPiece("timeout"));
        }
    }

    // repeat timer
    drop_timer_.expires_from_now(std::chrono::seconds(heart_beat_check_sec_));
    drop_timer_.async_wait(std::bind(&Gate::CheckHeartBeat, this));
}

void Gate::SessionStartRead(SessionPtr session)
{
    auto& head = session->head_;
    session->last_recv_time_ = time(NULL);
    asio::async_read(session->socket_, 
        asio::buffer(&head, sizeof(head)),
        std::bind(&Gate::HandleReadHead, this, session->serial_, _1, _2));
}

bool Gate::SessionWritePacket(SessionPtr session, ByteRange data)
{
    if (data.size() > 0 && data.size() < MAX_SEND_BYTES)
    {
        while (data.size() > MAX_PACKET_SIZE)
        {
            auto frame = data.subpiece(0, MAX_PACKET_SIZE);
            SessionWriteFrame(session, frame, PACKET_FRAME_PART);
            data.advance(MAX_PACKET_SIZE);
        }
        SessionWriteFrame(session, data, PACKET_FRAME_MSG);
        return true;
    }
    else
    {
        LOG(WARNING) << "packet too big: " << data.size();
        return false;
    }
}

void Gate::SessionWriteFrame(SessionPtr session, ByteRange frame, PacketType type)
{
    assert(frame.size() <= UINT16_MAX);
    CodecType codec = NO_COMPRESSION;
    if (frame.size() >= max_no_compress_size_)
    {
        codec = ZLIB;
    }
    auto out = compressServerPacket(codec, frame, xor_key_, type);
    if (out && !out->empty())
    {
        asio::async_write(session->socket_, 
            asio::buffer(out->buffer(), out->length()), 
            std::bind(&Gate::HandleWrite, this, session->serial_, _1, _2, out));
    }
}

void Gate::HandleReadHead(uint64_t serial, const std::error_code& ec, size_t bytes)
{
    auto session = GetSession(serial);
    if (!session)
    {
        return;
    }
    if (ec)
    {
        Kick(session);
        on_read_(ec.value(), serial, StringPiece(ec.message()));
        return;
    }

    auto& head = session->head_;
    XORBuffer(&head, sizeof(head), xor_key_);
    if (head.size > MAX_PACKET_SIZE)
    {
        Kick(session);
        auto errmsg = stringPrintf("invalid body size: %u", head.size);
        on_read_(ERR_INVALID_BODY_SIZE, serial, StringPiece(errmsg));
        return;
    }
    if (head.size == 0) // empty content means heartbeating
    {
        session->last_recv_time_ = time(NULL);
        SessionStartRead(session); // ready for packet content body
    }
    else
    {
        auto& buffer = session->recv_buf_;
        if (buffer.size() < head.size)
        {
            buffer.resize(head.size);
        }
        asio::async_read(session->socket_, 
            asio::buffer(buffer.data(), head.size),
            std::bind(&Gate::HandleReadBody, this, serial, _1, _2));
    }
}

void Gate::HandleReadBody(uint64_t serial, const std::error_code& ec, size_t bytes)
{
    auto session = GetSession(serial);
    if (!session)
    {
        return;
    }
    if (ec)
    {
        Kick(session);
        on_read_(ec.value(), serial, StringPiece(ec.message()));
        return;
    }

    auto& head = session->head_;
    auto& buffer = session->recv_buf_;
    uint8_t* data = buffer.data();
    XORBuffer(data, bytes, xor_key_);
    auto checksum = crc16(data, bytes);
    if (head.checksum == checksum)
    {
        session->last_recv_time_ = time(NULL);
        on_read_(0, serial, ByteRange(data, bytes));
        SessionStartRead(session); // waiting for another packet
    }
    else
    {
        Kick(session);
        auto errmsg = stringPrintf("invalid body checksum, expect 0x%x, but get 0x%x",
            checksum, head.checksum);
        on_read_(ERR_INVALID_CHECKSUM, serial, StringPiece(errmsg));
    }
}

void Gate::HandleWrite(uint64_t serial,
                       const std::error_code& ec,
                       size_t bytes, 
                       std::shared_ptr<IOBuf> buf)
{
    if (ec)
    {
        on_read_(ec.value(), serial, StringPiece(ec.message()));
    }
}

} // namespace net
