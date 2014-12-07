#pragma once

#include <malloc.h>
#include <vector>
#include "core/strings.h"
#include "core/checksum.h"
#include "packet.h"
#include "compression.h"

using namespace std::placeholders;

namespace net {

class Gate::Session : boost::noncopyable
{
public:
    Session(boost::asio::io_service& io_service, 
            uint32_t serial, 
            ReadCallback& callback);

    ~Session();

    void startRead();
    void write(ByteRange data);
    void close();

    std::string remoteAddress();
    boost::asio::ip::tcp::socket& socket() { return socket_; }
    uint32_t serial() const { return serial_; }
    bool isClosed() const { return closed_; }
    time_t lastRecvTime() const { return last_recv_time_; }

private:
    void writeFrame(ByteRange frame, bool more);
    void handleReadHead(const boost::system::error_code& ec, size_t bytes);
    void handleReadBody(const boost::system::error_code& ec, size_t bytes);
    void handleWrite(const boost::system::error_code& ec, size_t bytes, std::shared_ptr<IOBuf> buf);

private:
    boost::asio::ip::tcp::socket    socket_;
    uint32_t serial_ = 0;
    bool  closed_ = false;
    time_t last_recv_time_ = 0;
    ClientHeader  head_;
    std::vector<uint8_t> buffer_;
    ReadCallback&   callback_;
    std::string     remote_addr_;
};

//////////////////////////////////////////////////////////////////////////
Gate::Session::Session(boost::asio::io_service& io_service,
                       uint32_t serial,
                       ReadCallback& callback)
    : socket_(io_service), 
      serial_(serial), 
      callback_(callback)
{
    CHECK(callback);
    memset(&head_, 0, sizeof(head_));
    buffer_.reserve(kRecvBufReserveSize);
}

Gate::Session::~Session()
{
    close();
}

void Gate::Session::startRead()
{
    boost::asio::async_read(socket_, boost::asio::buffer(&head_, sizeof(head_)),
        std::bind(&Gate::Session::handleReadHead, this, _1, _2));
}

void Gate::Session::close()
{
    if (!closed_ && socket_.is_open())
    {
        socket_.shutdown(boost::asio::socket_base::shutdown_both);
        socket_.close();
        closed_ = true;
    }
}

void Gate::Session::handleReadHead(const boost::system::error_code& ec, size_t bytes)
{
    if (!ec)
    {
        if (head_.size > MAX_PACKET_SIZE)
        {
            auto errmsg = stringPrintf("invalid body size: %u", bytes);
            callback_(ERR_INVALID_BODY_SIZE, serial_, StringPiece(errmsg));
            return;
        }
        if (head_.size == 0) // empty content means heartbeating
        {
            last_recv_time_ = time(NULL);
            startRead();
        }
        else
        {
            if (buffer_.size() < head_.size)
            {
                buffer_.resize(head_.size);
            }
            boost::asio::async_read(socket_, boost::asio::buffer(buffer_.data(), head_.size),
                std::bind(&Gate::Session::handleReadBody, this, _1, _2));
        }
    }
    else
    {
        callback_(ec.value(), serial_, StringPiece(ec.message()));
    }
}

void Gate::Session::handleReadBody(const boost::system::error_code& ec, size_t bytes)
{
    if (!ec)
    {
        const uint8_t* data = buffer_.data();
        auto checksum = crc32c(data, bytes);
        if (head_.checksum == checksum)
        {
            last_recv_time_ = time(NULL);
            auto out = uncompressPacketFrame((CodecType)head_.codec, ByteRange(data, bytes));
            if (out)
            {
                callback_(0, serial_, out->byteRange());
            }
            startRead();
        }
        else
        {
            close();
            auto errmsg = stringPrintf("invalid body checksum, expect %x, but get %x", 
                checksum, head_.checksum);
            callback_(ERR_INVALID_CHECKSUM, serial_, StringPiece(errmsg));
        }
    }
    else
    {
        callback_(ec.value(), serial_, StringPiece(ec.message()));
    }
}

void Gate::Session::write(ByteRange data)
{
    if (data.size() > 0 && data.size() < MAX_SEND_BYTES)
    {
        while (data.size() > MAX_PACKET_SIZE)
        {
            auto frame = data.subpiece(0, MAX_PACKET_SIZE);
            writeFrame(frame, true);
            data.advance(MAX_PACKET_SIZE);
        }
        writeFrame(data, 0);
    }
    else
    {
        auto errmsg = stringPrintf("%u: invalid data size to send: %u", 
                prettyPrint(data.size(), PRETTY_BYTES));
        callback_(ERR_SND_SIZE_TOO_BIG, serial_, StringPiece(errmsg));
    }
}

void Gate::Session::writeFrame(ByteRange frame, bool more)
{
    assert(frame.size() <= UINT16_MAX);
    auto out = compressServerPacket(frame, more);
    if (out && !out->empty())
    {
        boost::asio::async_write(socket_, boost::asio::buffer(out->buffer(), 
            out->length()), std::bind(&Gate::Session::handleWrite, this, _1, _2, out));
    }
}


void Gate::Session::handleWrite(const boost::system::error_code& ec,
                                size_t bytes, 
                                std::shared_ptr<IOBuf> buf)
{
    if (ec)
    {
        close();
        callback_(ec.value(), serial_, StringPiece(ec.message()));
    }
}

std::string Gate::Session::remoteAddress()
{
    if (remote_addr_.empty())
    {
        auto endpoint = socket_.remote_endpoint();
        remote_addr_ = endpoint.address().to_string();
    }
    return remote_addr_;
}

} // namespace net
