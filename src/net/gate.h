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
#include "packet.h"

namespace net {

typedef std::function<void(int, uint32_t, ByteRange)>   ReadCallback;

class Gate : boost::noncopyable
{
    class Session;
    typedef std::shared_ptr<Session> SessionPtr;

public:
    explicit Gate(boost::asio::io_service& io_service, 
                  uint32_t max_connections = DEFAULT_MAX_CONNECTIONS,
                  uint32_t heart_beat_sec = DEFAULT_MAX_HEARTBEAT_SEC);
    ~Gate();

    void start(const std::string& host, uint16_t port, ReadCallback callback);

    void stop();

    void send(uint32_t serial, ByteRange data);
    void send(uint32_t serial, const std::string& str)
    {
        send(serial, ByteRange(StringPiece(str)));
    }
    void send(uint32_t serial, const void* data, size_t size)
    {
        assert(data && size > 0);
        send(serial, ByteRange(reinterpret_cast<const uint8_t*>(data), size));
    }

    bool kick(uint32_t serial);
    void kickAll();

    uint32_t size() const { return sessions_.size(); }

    void denyAddress(const std::string& address);
    void allowAddress(const std::string& address);

private:
    uint32_t nextSerial();
    void startAccept();
    void handleAccept(const boost::system::error_code& err, SessionPtr ptr);
    void checkHeartBeat();

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

typedef std::shared_ptr<Gate>   GatePtr;

} // namespace net
