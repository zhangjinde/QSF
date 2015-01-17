#include "Client.h"
#include <malloc.h>
#include <ctime>
#include <chrono>
#include <asio.hpp>
#include "core/Strings.h"
#include "core/Logging.h"
#include "Compression.h"

using namespace std::placeholders;

enum
{
    DEFAULT_RECV_BUF_SIZE = 128,
    DEFAULT_HEATBEAT_CHECK_SEC = 5,
};

inline void XORHeader(void* data, size_t size, uint8_t key)
{
    uint8_t* p = reinterpret_cast<uint8_t*>(data);
    for (size_t i = 0; i < size; i++)
    {
        *(p + i) = *(p + i) ^ key;
    }
}

namespace net {

Client::Client(asio::io_service& io_service, 
               uint32_t heart_beat_sec_,
               uint16_t no_compress_size,
               uint8_t xor_key)
    : socket_(io_service), 
      heart_beat_sec_(heart_beat_sec_),
      heart_beat_(io_service, std::chrono::seconds(DEFAULT_HEATBEAT_CHECK_SEC)),
      no_compress_size_(no_compress_size),
      xor_key_(xor_key)

{
    buffer_.reserve(DEFAULT_RECV_BUF_SIZE);
    buffer_more_.reserve(DEFAULT_RECV_BUF_SIZE);
}

Client::~Client()
{
    Stop();
}

void Client::Stop()
{
    socket_.close();
}

void Client::Connect(const std::string& host, uint16_t port)
{
    using namespace asio::ip;
    tcp::endpoint endpoint(address::from_string(host), port);
    socket_.connect(endpoint);
}

void Client::Connect(const std::string& host,
                     uint16_t port, 
                     ConnectCallback callback)
{
    using namespace asio::ip;
    tcp::endpoint endpoint(address::from_string(host), port);
    socket_.async_connect(endpoint, callback);
}

void Client::StartRead(ReadCallback callback)
{
    on_read_ = callback;
    heart_beat_.async_wait(std::bind(&Client::HeartBeating, this));
    ReadHead();
}

void Client::Write(ByteRange data)
{
    if (data.size() > MAX_PACKET_SIZE)
    {
        LOG(ERROR) << "too big size to send: " << prettyPrint(data.size(), PRETTY_BYTES);
        return;
    }
    auto out = compressClientPacket(NO_COMPRESSION, data, xor_key_);
    if (out)
    {
        asio::async_write(socket_, 
            asio::buffer(out->buffer(), out->length()),
            std::bind(&Client::HandleSend, this, _1, _2, out));
    }
}

void Client::ReadHead()
{
    asio::async_read(socket_, 
        asio::buffer(&head_, sizeof(head_)),
        std::bind(&Client::HandleReadHead, this, _1, _2));
}

void Client::HandleReadHead(const std::error_code& ec, size_t bytes)
{
    if (!ec)
    {
        XORHeader(&head_, sizeof(head_), xor_key_);
        if (head_.size > 0)
        {
            buffer_.resize(head_.size);
            asio::async_read(socket_, 
                asio::buffer(buffer_.data(), head_.size),
                std::bind(&Client::HandleReadBody, this, _1, _2));
        }
    }
    else
    {
        LOG(ERROR) << ec.message();
    }
}

void Client::HandleReadBody(const std::error_code& ec, size_t bytes)
{
    if (!ec)
    {
        if (head_.more == PACKET_FRAME_MSG && buffer_more_.size() == 0) // hot path
        {
            auto buf = uncompressPacketFrame((CodecType)head_.codec, 
                ByteRange(buffer_.data(), bytes), xor_key_);
            if (buf)
            {
                on_read_(buf->byteRange());
            }
        }
        else // PACKET_FRAME_PART, part of a msg
        {
            auto buf = uncompressPacketFrame((CodecType)head_.codec, 
                ByteRange(buffer_.data(), bytes), xor_key_);
            if (buf)
            {
                auto range = buf->byteRange();
                auto oldsize = buffer_more_.size();
                buffer_more_.resize(oldsize + range.size());
                memcpy(buffer_more_.data() + oldsize, range.data(), range.size());
            }
            if (head_.more == PACKET_FRAME_MSG)
            {
                on_read_(ByteRange(buffer_more_.data(), buffer_more_.size()));
                buffer_more_.resize(0);
            }
        }
        ReadHead();
    }
    else
    {
        LOG(ERROR) << ec.message();
    }
}

void Client::HandleSend(const std::error_code& ec, 
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

void Client::HeartBeating()
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
            std::bind(&Client::HandleSend, this, _1, _2, out));
    }
    heart_beat_.expires_from_now(std::chrono::seconds(heart_beat_sec_/2));
    heart_beat_.async_wait(std::bind(&Client::HeartBeating, this));
}

} // namespace net
