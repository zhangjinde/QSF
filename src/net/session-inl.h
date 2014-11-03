#pragma once

#include <malloc.h>
#include <vector>
#include "core/strings.h"
#include "packet.h"
#include "checksum.h"

using namespace std::placeholders;

class Gate::Session : boost::noncopyable
{
public:
    Session(boost::asio::io_service& io_service, 
            uint32_t serial, 
            ReadCallback& callback);

    ~Session();

    void AsynRead();
    void AsynWrite(ByteRange data);
    void Close(const boost::system::error_code& ec = boost::asio::error::connection_aborted);

    std::string GetAddress();
    boost::asio::ip::tcp::socket& GetSocket() { return socket_; }
    uint32_t GetSerial() const { return serial_; }
    bool IsClosed() const { return closed_; }
    time_t GetLastRecvTime() const { return last_recv_time_; }

private:
    void AsynWriteImpl(ByteRange data, uint8_t more);
    void HandleReadHead(const boost::system::error_code& ec, size_t bytes);
    void HandleReadBody(const boost::system::error_code& ec, size_t bytes);
    void HandleWrite(const boost::system::error_code& ec, size_t bytes, uint8_t* buf);

private:
    boost::asio::ip::tcp::socket    socket_;
    uint32_t serial_ = 0;
    bool  closed_ = false;
    time_t last_recv_time_ = 0;
    ClientHeader  head_;
    std::vector<uint8_t> buffer_;
    ReadCallback&   callback_;
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
    Close();
}

void Gate::Session::AsynRead()
{
    boost::asio::async_read(socket_, boost::asio::buffer(&head_, sizeof(head_)),
        std::bind(&Gate::Session::HandleReadHead, this, _1, _2));
}

void Gate::Session::Close(const boost::system::error_code& ec)
{
    if (!closed_ && socket_.is_open())
    {
        socket_.shutdown(boost::asio::socket_base::shutdown_both);
        socket_.close();
        closed_ = true;

        StringPiece msg(ec.message());
        callback_(serial_, ec.value(), msg);
    }
}

void Gate::Session::HandleReadHead(const boost::system::error_code& ec, size_t bytes)
{
    if (!ec)
    {
        if (head_.size > MAX_PACKET_SIZE)
        {
            Close(boost::asio::error::message_size);
            return;
        }
        if (head_.size == 0) // empty content means heartbeating
        {
            last_recv_time_ = time(NULL);
            AsynRead();
        }
        else
        {
            if (buffer_.size() < head_.size)
            {
                buffer_.resize(head_.size);
            }
            boost::asio::async_read(socket_, boost::asio::buffer(buffer_.data(), head_.size),
                std::bind(&Gate::Session::HandleReadBody, this, _1, _2));
        }
    }
    else
    {
        LOG(DEBUG) << ec.message();
    }
}

void Gate::Session::HandleReadBody(const boost::system::error_code& ec, size_t bytes)
{
    if (!ec)
    {
        const uint8_t* data = buffer_.data();
        auto checksum = crc16(data, bytes);
        if (head_.checksum == checksum)
        {
            last_recv_time_ = time(NULL);
            callback_(serial_, 0, ByteRange(data, bytes));
            AsynRead();
        }
        else
        {
            Close(boost::asio::error::invalid_argument);
        }
    }
    else
    {
        LOG(DEBUG) << ec.message();
    }
}

void Gate::Session::AsynWrite(ByteRange data)
{
    if (data.size() > 0 && data.size() < MAX_SEND_BYTES)
    {
        while (data.size() > MAX_PACKET_SIZE)
        {
            auto frame = data.subpiece(0, MAX_PACKET_SIZE);
            AsynWriteImpl(frame, 1);
            data.advance(MAX_PACKET_SIZE);
        }
        AsynWriteImpl(data, 0);
    }
    else
    {
        LOG(DEBUG) << serial_ << " invalid data buffer size: " 
            << prettyPrint(data.size(), PRETTY_BYTES);
    }
}

void Gate::Session::AsynWriteImpl(ByteRange data, uint8_t more)
{
    assert(data.size() <= UINT16_MAX);
    size_t size = data.size() + sizeof(ServerHeader);
    uint8_t* buf = reinterpret_cast<uint8_t*>(malloc(size));
    if (buf == nullptr)
    {
        //LOG(ERROR) << serial_ << ", out of memory with size: " << size;
        return;
    }
    ServerHeader* head = reinterpret_cast<ServerHeader*>(buf);
    head->size = static_cast<uint16_t>(data.size());
    head->codec = NO_COMPRESSION;
    head->more = more;
    memcpy(buf + sizeof(*head), data.data(), data.size());
    boost::asio::async_write(socket_, boost::asio::buffer(buf, size),
        std::bind(&Gate::Session::HandleWrite, this, _1, _2, buf));
}


void Gate::Session::HandleWrite(const boost::system::error_code& ec,
                                size_t bytes, 
                                uint8_t* buf)
{
    if (buf)
    {
        free(buf);
    }
}

std::string Gate::Session::GetAddress()
{
    auto endpoint = socket_.remote_endpoint();
    return endpoint.address().to_string();
}
