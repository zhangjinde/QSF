#include "client.h"
#include <malloc.h>
#include <ctime>
#include <chrono>
#include <boost/asio.hpp>
#include "core/strings.h"
#include "core/logging.h"
#include "checksum.h"

using namespace std::placeholders;

Client::Client(boost::asio::io_service& io_service, uint32_t heart_beat_sec_)
    : socket_(io_service), 
      heart_beat_sec_(heart_beat_sec_),
      heart_beat_(io_service, std::chrono::seconds(1))
{
    buffer_.reserve(kRecvBufReserveSize);
    buffer_more_.reserve(kRecvBufReserveSize);
}

Client::~Client()
{
}

void Client::Connect(const std::string& host, uint16_t port)
{
    using namespace boost::asio;
    ip::tcp::endpoint endpoint(ip::address::from_string(host), port);
    socket_.connect(endpoint);
}

void Client::Connect(const std::string& host,
                     uint16_t port, 
                     ConnectCallback callback)
{
    using namespace boost::asio;
    ip::tcp::endpoint endpoint(ip::address::from_string(host), port);
    socket_.async_connect(endpoint, callback);
}

void Client::StartRead(ReadCallback callback)
{
    on_read_ = callback;
    heart_beat_.async_wait(std::bind(&Client::HeartBeating, this));
    AsynReadHead();
}

void Client::Send(ByteRange data)
{
    if (data.size() > MAX_PACKET_SIZE)
    {
        LOG(ERROR) << "too big size to send: " << prettyPrint(data.size(), PRETTY_BYTES);
        return;
    }
    size_t size = data.size() + sizeof(ClientHeader);
    uint8_t* buf = reinterpret_cast<uint8_t*>(malloc(size));
    if (buf == nullptr)
    {
        //LOG(ERROR) << serial_ << ", out of memory with size: " << size;
        return;
    }
    ClientHeader* head = reinterpret_cast<ClientHeader*>(buf);
    head->size = static_cast<uint16_t>(data.size());
    head->checksum = crc16(data.data(), data.size());
    memcpy(buf + sizeof(*head), data.data(), data.size());
    boost::asio::async_write(socket_, boost::asio::buffer(buf, size),
        std::bind(&Client::HandleSend, this, _1, _2, buf));
}

void Client::AsynReadHead()
{
    boost::asio::async_read(socket_, boost::asio::buffer(&head_, sizeof(head_)),
        std::bind(&Client::HandleReadHead, this, _1, _2));
}

void Client::HandleReadHead(const boost::system::error_code& ec, size_t bytes)
{
    if (!ec)
    {
        if (head_.size > 0)
        {
            buffer_.resize(head_.size);
            boost::asio::async_read(socket_, boost::asio::buffer(buffer_.data(), head_.size),
                std::bind(&Client::HandleReadBody, this, _1, _2));
        }
    }
    else
    {
        LOG(ERROR) << ec.message();
    }
}

void Client::HandleReadBody(const boost::system::error_code& ec, size_t bytes)
{
    if (!ec)
    {
        if (head_.more == 0 && buffer_more_.size() == 0)
        {
            std::swap(buffer_more_, buffer_);
        }
        else
        {
            auto oldsize = buffer_more_.size();
            buffer_more_.resize(oldsize + bytes);
            memcpy(buffer_more_.data() + oldsize, buffer_.data(), bytes);
        }
        if (head_.more == 0)
        {
            on_read_(ByteRange(buffer_more_.data(), buffer_more_.size()));
            buffer_more_.resize(0);
        }
        AsynReadHead();
    }
    else
    {
        LOG(ERROR) << ec.message();
    }
}

void Client::HandleSend(const boost::system::error_code& ec, size_t bytes, uint8_t* buf)
{
    free(buf);
    if (ec)
    {
        LOG(ERROR) << ec.message();
    }
    last_send_time_ = time(NULL);
}

void Client::HeartBeating()
{
    time_t now = time(NULL);
    if (now - last_send_time_ >= heart_beat_sec_)
    {
        uint8_t* buf = reinterpret_cast<uint8_t*>(malloc(sizeof(ClientHeader)));
        if (buf == nullptr)
        {
            //LOG(ERROR) << serial_ << ", out of memory with size: " << size;
            return;
        }
        ClientHeader* head = reinterpret_cast<ClientHeader*>(buf);
        head->size = 0;
        head->checksum = 0;
        boost::asio::async_write(socket_, boost::asio::buffer(head, sizeof(*head)),
            std::bind(&Client::HandleSend, this, _1, _2, buf));
    }
    heart_beat_.expires_from_now(std::chrono::seconds(heart_beat_sec_/2));
    heart_beat_.async_wait(std::bind(&Client::HeartBeating, this));
}