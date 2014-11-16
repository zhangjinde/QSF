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
    void close(const boost::system::error_code& ec = boost::asio::error::connection_aborted);

    std::string remoteAddress();
    boost::asio::ip::tcp::socket& socket() { return socket_; }
    uint32_t serial() const { return serial_; }
    bool isClosed() const { return closed_; }
    time_t lastRecvTime() const { return last_recv_time_; }

private:
    void writeFrame(ByteRange frame, uint8_t more);
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

void Gate::Session::close(const boost::system::error_code& ec)
{
    if (!closed_ && socket_.is_open())
    {
        socket_.shutdown(boost::asio::socket_base::shutdown_both);
        socket_.close();
        closed_ = true;

        StringPiece msg(ec.message());
        callback_(ec.value(), serial_, msg);
    }
}

void Gate::Session::handleReadHead(const boost::system::error_code& ec, size_t bytes)
{
    if (!ec)
    {
        if (head_.size > MAX_PACKET_SIZE)
        {
            close(boost::asio::error::message_size);
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
        LOG(INFO) << ec.message();
    }
}

void Gate::Session::handleReadBody(const boost::system::error_code& ec, size_t bytes)
{
    if (!ec)
    {
        const uint8_t* data = buffer_.data();
        auto checksum = crc16(data, bytes);
        if (head_.checksum == checksum)
        {
            last_recv_time_ = time(NULL);
            auto buf = uncompress(ZLIB, ByteRange(data, bytes));
            callback_(0, serial_, buf->byteRange());
            startRead();
        }
        else
        {
            close(boost::asio::error::invalid_argument);
        }
    }
    else
    {
        LOG(INFO) << ec.message();
    }
}

void Gate::Session::write(ByteRange data)
{
    if (data.size() > 0 && data.size() < MAX_SEND_BYTES)
    {
        while (data.size() > MAX_PACKET_SIZE)
        {
            auto frame = data.subpiece(0, MAX_PACKET_SIZE);
            writeFrame(frame, 1);
            data.advance(MAX_PACKET_SIZE);
        }
        writeFrame(data, 0);
    }
    else
    {
        LOG(DEBUG) << serial_ << " invalid data buffer size: " 
            << prettyPrint(data.size(), PRETTY_BYTES);
    }
}

void Gate::Session::writeFrame(ByteRange frame, uint8_t more)
{
    assert(frame.size() <= UINT16_MAX);
    const size_t head_size = sizeof(ServerHeader);
    auto out = compress(ZLIB, frame, head_size);
    ServerHeader* head = reinterpret_cast<ServerHeader*>(out->buffer());
    head->size = static_cast<uint16_t>(frame.size());
    head->codec = ZLIB;
    head->more = more;
    boost::asio::async_write(socket_, boost::asio::buffer(out->buffer(), out->length()),
        std::bind(&Gate::Session::handleWrite, this, _1, _2, out));
}


void Gate::Session::handleWrite(const boost::system::error_code& ec,
                                size_t bytes, 
                                std::shared_ptr<IOBuf> buf)
{
    if (ec)
    {
        LOG(INFO) << ec.message();
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
