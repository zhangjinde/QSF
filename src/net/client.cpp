#include "client.h"
#include <malloc.h>
#include <ctime>
#include <chrono>
#include <boost/asio.hpp>
#include "core/strings.h"
#include "core/logging.h"
#include "core/checksum.h"
#include "compression.h"

using namespace std::placeholders;

namespace net {

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

void Client::connect(const std::string& host, uint16_t port)
{
    using namespace boost::asio;
    ip::tcp::endpoint endpoint(ip::address::from_string(host), port);
    socket_.connect(endpoint);
}

void Client::connect(const std::string& host,
                     uint16_t port, 
                     ConnectCallback callback)
{
    using namespace boost::asio;
    ip::tcp::endpoint endpoint(ip::address::from_string(host), port);
    socket_.async_connect(endpoint, callback);
}

void Client::startRead(ReadCallback callback)
{
    on_read_ = callback;
    heart_beat_.async_wait(std::bind(&Client::heartBeating, this));
    readHead();
}

void Client::send(ByteRange data)
{
    if (data.size() > MAX_PACKET_SIZE)
    {
        LOG(ERROR) << "too big size to send: " << prettyPrint(data.size(), PRETTY_BYTES);
        return;
    }
    const uint32_t head_size = sizeof(ClientHeader);
    auto out = compress(ZLIB, data, head_size);
    if (out->empty())
    {
        //LOG(ERROR) << serial_ << ", out of memory with size: " << size;
        return;
    }
    ClientHeader* head = reinterpret_cast<ClientHeader*>(out->buffer());
    size_t body_size = out->length() - head_size;
    head->size = static_cast<uint16_t>(body_size);
    head->checksum = crc16(out->buffer() + head_size, body_size);
    boost::asio::async_write(socket_, boost::asio::buffer(out->buffer(), out->length()),
        std::bind(&Client::handleSend, this, _1, _2, out));
}

void Client::readHead()
{
    boost::asio::async_read(socket_, boost::asio::buffer(&head_, sizeof(head_)),
        std::bind(&Client::handleReadHead, this, _1, _2));
}

void Client::handleReadHead(const boost::system::error_code& ec, size_t bytes)
{
    if (!ec)
    {
        if (head_.size > 0)
        {
            buffer_.resize(head_.size);
            boost::asio::async_read(socket_, boost::asio::buffer(buffer_.data(), head_.size),
                std::bind(&Client::handleReadBody, this, _1, _2));
        }
    }
    else
    {
        LOG(ERROR) << ec.message();
    }
}

void Client::handleReadBody(const boost::system::error_code& ec, size_t bytes)
{
    if (!ec)
    {
        if (head_.more == 0 && buffer_more_.size() == 0) // most case
        {
            std::swap(buffer_more_, buffer_);
        }
        else // framed packet
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
        readHead();
    }
    else
    {
        LOG(ERROR) << ec.message();
    }
}

void Client::handleSend(const boost::system::error_code& ec, size_t bytes, std::shared_ptr<IOBuf> buf)
{
    if (!ec)
    {
        last_send_time_ = time(NULL);
    }
    else
    {
        LOG(ERROR) << ec.message();
    }
}

void Client::heartBeating()
{
    time_t now = time(NULL);
    if (now - last_send_time_ >= heart_beat_sec_)
    {
        auto out = IOBuf::create(sizeof(ClientHeader));
        ClientHeader* head = reinterpret_cast<ClientHeader*>(out->buffer());
        head->size = 0;
        head->checksum = 0;
        out->append(sizeof(*head));
        boost::asio::async_write(socket_, boost::asio::buffer(out->buffer(), out->length()),
            std::bind(&Client::handleSend, this, _1, _2, out));
    }
    heart_beat_.expires_from_now(std::chrono::seconds(heart_beat_sec_/2));
    heart_beat_.async_wait(std::bind(&Client::heartBeating, this));
}

} // namespace net
