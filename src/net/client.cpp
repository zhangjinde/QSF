#include "client.h"
#include <malloc.h>
#include <ctime>
#include <chrono>
#include <asio.hpp>
#include "core/strings.h"
#include "core/logging.h"

#include "compression.h"

using namespace std::placeholders;

enum
{
    DEFAULT_RECV_BUF_SIZE = 128,
    DEFAULT_HEATBEAT_CHECK_SEC = 5,
};

namespace net {

Client::Client(asio::io_service& io_service, 
               uint32_t heart_beat_sec_,
               uint16_t no_compress_size)
    : socket_(io_service), 
      heart_beat_sec_(heart_beat_sec_),
      heart_beat_(io_service, std::chrono::seconds(DEFAULT_HEATBEAT_CHECK_SEC)),
      no_compress_size_(no_compress_size)

{
    buffer_.reserve(DEFAULT_RECV_BUF_SIZE);
    buffer_more_.reserve(DEFAULT_RECV_BUF_SIZE);
}

Client::~Client()
{
    stop();
}

void Client::stop()
{
    socket_.close();
}

void Client::connect(const std::string& host, uint16_t port)
{
    using namespace asio::ip;
    tcp::endpoint endpoint(address::from_string(host), port);
    socket_.connect(endpoint);
}

void Client::connect(const std::string& host,
                     uint16_t port, 
                     ConnectCallback callback)
{
    using namespace asio::ip;
    tcp::endpoint endpoint(address::from_string(host), port);
    socket_.async_connect(endpoint, callback);
}

void Client::startRead(ReadCallback callback)
{
    on_read_ = callback;
    heart_beat_.async_wait(std::bind(&Client::heartBeating, this));
    readHead();
}

void Client::write(ByteRange data)
{
    if (data.size() > MAX_PACKET_SIZE)
    {
        LOG(ERROR) << "too big size to send: " << prettyPrint(data.size(), PRETTY_BYTES);
        return;
    }
    auto out = compressClientPacket(NO_COMPRESSION, data);
    if (out)
    {
        asio::async_write(socket_, 
            asio::buffer(out->buffer(), out->length()),
            std::bind(&Client::handleSend, this, _1, _2, out));
    }
}

void Client::readHead()
{
    asio::async_read(socket_, 
        asio::buffer(&head_, sizeof(head_)),
        std::bind(&Client::handleReadHead, this, _1, _2));
}

void Client::handleReadHead(const std::error_code& ec, size_t bytes)
{
    if (!ec)
    {
        if (head_.size > 0)
        {
            buffer_.resize(head_.size);
            asio::async_read(socket_, 
                asio::buffer(buffer_.data(), head_.size),
                std::bind(&Client::handleReadBody, this, _1, _2));
        }
    }
    else
    {
        LOG(ERROR) << ec.message();
    }
}

void Client::handleReadBody(const std::error_code& ec, size_t bytes)
{
    if (!ec)
    {
        if (head_.more == 0 && buffer_more_.size() == 0) // hot path
        {
            auto buf = uncompressPacketFrame((CodecType)head_.codec, 
                ByteRange(buffer_.data(), bytes));
            if (buf)
            {
                on_read_(buf->byteRange());
            }
        }
        else // framed packet
        {
            auto buf = uncompressPacketFrame((CodecType)head_.codec, 
                ByteRange(buffer_.data(), bytes));
            if (buf)
            {
                auto range = buf->byteRange();
                auto oldsize = buffer_more_.size();
                buffer_more_.resize(oldsize + range.size());
                memcpy(buffer_more_.data() + oldsize, range.data(), range.size());
            }
            if (head_.more == 0)
            {
                on_read_(ByteRange(buffer_more_.data(), buffer_more_.size()));
                buffer_more_.resize(0);
            }
        }
        readHead();
    }
    else
    {
        LOG(ERROR) << ec.message();
    }
}

void Client::handleSend(const std::error_code& ec, 
                        size_t bytes, 
                        std::shared_ptr<IOBuf> buf)
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
        asio::async_write(socket_, 
            asio::buffer(out->buffer(), out->length()),
            std::bind(&Client::handleSend, this, _1, _2, out));
    }
    heart_beat_.expires_from_now(std::chrono::seconds(heart_beat_sec_/2));
    heart_beat_.async_wait(std::bind(&Client::heartBeating, this));
}

} // namespace net
