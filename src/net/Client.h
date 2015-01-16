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
                    uint16_t no_compress_size = DEFAULT_NO_COMPRESSION_SIZE);
    ~Client();

    Client(const Client&) = delete;
    Client& operator = (const Client&) = delete;

    void connect(const std::string& host, uint16_t port);
    void connect(const std::string& host, uint16_t port, ConnectCallback callback);

    void startRead(ReadCallback callback);
    void write(ByteRange data);
    void write(const std::string& str) { write(ByteRange(StringPiece(str))); }
    void write(const void* data, size_t size)
    {
        assert(data && size > 0);
        write(ByteRange(reinterpret_cast<const uint8_t*>(data), size));
    }

    void stop();

private:
    void readHead();
    void handleReadHead(const std::error_code& ec, size_t bytes);
    void handleReadBody(const std::error_code& ec, size_t bytes);
    void handleSend(const std::error_code& ec, 
                    size_t bytes, 
                    std::shared_ptr<IOBuf> buf);
    void heartBeating();

private:
    asio::ip::tcp::socket    socket_;
    asio::steady_timer       heart_beat_;

    ServerHeader            head_;
    std::vector<uint8_t>    buffer_;
    std::vector<uint8_t>    buffer_more_;
    ReadCallback            on_read_;

    time_t  last_send_time_ = 0;

    const uint32_t heart_beat_sec_;
    const uint16_t no_compress_size_;
};

typedef std::shared_ptr<Client> ClientPtr;

} // namespace net
