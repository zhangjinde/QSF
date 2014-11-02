#pragma once

#include <cstdint>
#include <ctime>
#include <cassert>
#include <string>
#include <memory>
#include <unordered_set>
#include <unordered_map>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/system/error_code.hpp>
#include <boost/noncopyable.hpp>
#include "core/range.h"

typedef std::function<void(uint32_t, int, ByteRange)>   ReadCallback;

class Gate : boost::noncopyable
{
    class Session;
    typedef std::shared_ptr<Session> SessionPtr;

public:
    Gate(boost::asio::io_service& io_service, 
         uint32_t max_connections,
         uint32_t heart_beat_sec);
    ~Gate();

    void Start(const std::string& host, uint16_t port, ReadCallback callback);

    void Stop();

    void Send(uint32_t serial, ByteRange data);
    void Send(uint32_t serial, const void* data, size_t size)
    {
        assert(data && size > 0);
        Send(serial, ByteRange(reinterpret_cast<const uint8_t*>(data), size));
    }

    bool Kick(uint32_t serial);

    void DenyAddress(const std::string& address);
    void AllowAddress(const std::string& address);

private:
    uint32_t NextSessionSerial();

    void StartAccept();
    void HandleAccept(const boost::system::error_code& err, SessionPtr ptr);
    void CheckHeartBeat();

private:
    boost::asio::io_service&        io_service_;
    boost::asio::ip::tcp::acceptor  acceptor_;
    boost::asio::steady_timer       drop_timer_;

    ReadCallback    on_read_;
    
    uint32_t current_serial_ = 1000;
    std::unordered_map<uint32_t, SessionPtr> sessions_;

    const uint32_t heart_beat_sec_;
    const uint32_t max_connections_;

    std::unordered_set<std::string>  black_list_;
};