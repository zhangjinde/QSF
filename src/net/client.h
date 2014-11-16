#pragma once

#include <cstdint>
#include <cassert>
#include <string>
#include <vector>
#include <functional>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/system/error_code.hpp>
#include <boost/noncopyable.hpp>
#include "core/range.h"
#include "packet.h"
#include "iobuf.h"

namespace net {

class Client : boost::noncopyable
{
public:
    typedef std::function<void(const boost::system::error_code& ec)> ConnectCallback;
    typedef std::function<void(ByteRange)>  ReadCallback;

public:
    explicit Client(boost::asio::io_service& io_service, 
                    uint32_t heart_beat_sec = DEFAULT_MAX_HEARTBEAT_SEC);
    ~Client();

    void connect(const std::string& host, uint16_t port);
    void connect(const std::string& host, uint16_t port, ConnectCallback callback);

    void startRead(ReadCallback callback);
    void send(ByteRange data);
    void send(const void* data, size_t size)
    {
        assert(data && size > 0);
        send(ByteRange(reinterpret_cast<const uint8_t*>(data), size));
    }

private:
    void readHead();
    void handleReadHead(const boost::system::error_code& ec, size_t bytes);
    void handleReadBody(const boost::system::error_code& ec, size_t bytes);
    void handleSend(const boost::system::error_code& ec, size_t bytes, 
                    std::shared_ptr<IOBuf> buf);
    void heartBeating();

private:
    boost::asio::ip::tcp::socket    socket_;
    boost::asio::steady_timer       heart_beat_;

    ServerHeader            head_;
    std::vector<uint8_t>    buffer_;
    std::vector<uint8_t>    buffer_more_;
    ReadCallback            on_read_;

    time_t  last_send_time_ = 0;
    const uint32_t heart_beat_sec_;
};

} // namespace net
