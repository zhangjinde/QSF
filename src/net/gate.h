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
#include "packet.h"
#include "iobuf.h"

namespace net {

typedef std::function<void(int, uint32_t, ByteRange)>   ReadCallback;

class Gate : boost::noncopyable
{
    struct Session;
    typedef std::shared_ptr<Session> SessionPtr;

public:
    explicit Gate(boost::asio::io_service& io_service, 
                  uint32_t max_connections = DEFAULT_MAX_CONNECTIONS,
                  uint32_t heart_beat_sec = DEFAULT_MAX_HEARTBEAT_SEC,
                  uint32_t heart_beat_check_sec = DEFAULT_HEARTBEAT_CHECK_SEC,
                  uint16_t max_no_compress_size = DEFAULT_NO_COMPRESSION_SIZE);
    ~Gate();

    void start(const std::string& host, uint16_t port, ReadCallback callback);

    void stop();

    bool send(uint32_t serial, ByteRange data);
    bool send(uint32_t serial, const std::string& str)
    {
        return send(serial, ByteRange(StringPiece(str)));
    }
    bool send(uint32_t serial, const void* buf, size_t size)
    {
        assert(buf && size > 0);
        return send(serial, ByteRange(reinterpret_cast<const uint8_t*>(buf), size));
    }
    void sendAll(const void* buf, size_t size);

    bool kick(uint32_t serial);
    void kickAll();

    uint32_t size() const { return sessions_.size(); }

    void denyAddress(const std::string& address);
    void allowAddress(const std::string& address);

private:
    uint32_t nextSerial();
    SessionPtr getSession(uint32_t serial);
    void kick(SessionPtr session);
    void checkHeartBeat();
    void startAccept();
    void sessionStartRead(SessionPtr session);
    bool sessionWritePacket(SessionPtr session, ByteRange data);
    void sessionWriteFrame(SessionPtr session, ByteRange frame, bool more);

    void handleAccept(const boost::system::error_code& err, SessionPtr ptr);
    void handleReadHead(uint32_t serial, const boost::system::error_code& ec, size_t bytes);
    void handleReadBody(uint32_t serial, const boost::system::error_code& ec, size_t bytes);
    void handleWrite(uint32_t serial, const boost::system::error_code& ec, size_t bytes, 
                     std::shared_ptr<IOBuf> buf);

private:
    boost::asio::io_service&        io_service_;
    boost::asio::ip::tcp::acceptor  acceptor_;
    boost::asio::steady_timer       drop_timer_;

    ReadCallback    on_read_;
    
    uint32_t current_serial_ = 1000;
    std::unordered_map<uint32_t, SessionPtr> sessions_;

    const uint32_t heart_beat_sec_;
    const uint32_t heart_beat_check_sec_;
    const uint32_t max_connections_;
    const uint16_t max_no_compress_size_;

    std::unordered_set<std::string>  black_list_;
};

typedef std::shared_ptr<Gate>   GatePtr;

} // namespace net
