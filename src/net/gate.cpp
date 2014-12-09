#include "gate.h"
#include <ctime>
#include <cinttypes>
#include <functional>
#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>
#include "core/logging.h"
#include "core/strings.h"
#include "core/checksum.h"
#include "compression.h"

using namespace std::placeholders;

namespace net {

const int kDeadConnectionReserveSize = 8;
const int kSessionRecvBufReserveSize = 64;

// Connection session object
struct Gate::Session : boost::noncopyable
{
    uint32_t        serial_ = 0;
    bool            closed_ = false;
    time_t          last_recv_time_ = 0;
    ClientHeader    head_;

    std::vector<uint8_t>            recv_buf_;
    std::string                     remote_addr_;
    boost::asio::ip::tcp::socket    socket_;

    Session(boost::asio::io_service& io_service, uint32_t serial)
        : socket_(io_service), serial_(serial)
    {
        recv_buf_.reserve(kSessionRecvBufReserveSize);
    }
};


//////////////////////////////////////////////////////////////////////////

Gate::Gate(boost::asio::io_service& io_service, 
           uint32_t max_connections, 
           uint32_t heart_beat_sec,
           uint32_t heart_beat_check_sec)
    : io_service_(io_service), 
      acceptor_(io_service), 
      drop_timer_(io_service),
      max_connections_(max_connections),
      heart_beat_sec_(heart_beat_sec),
      heart_beat_check_sec_(heart_beat_check_sec)
{
}

Gate::~Gate()
{
    stop();
}

void Gate::start(const std::string& host, uint16_t port, ReadCallback callback)
{
    using namespace boost::asio::ip;
    assert(callback);
    on_read_ = callback;

    tcp::endpoint endpoint(address::from_string(host), port);
    acceptor_.open(endpoint.protocol());
    acceptor_.set_option(tcp::acceptor::reuse_address(true));
    acceptor_.bind(endpoint);
    acceptor_.listen();

    drop_timer_.expires_from_now(std::chrono::seconds(heart_beat_check_sec_));
    drop_timer_.async_wait(std::bind(&Gate::checkHeartBeat, this));

    startAccept();
}

void Gate::stop()
{
    drop_timer_.cancel();
    acceptor_.close();
    black_list_.clear();
    sessions_.clear();
}

Gate::SessionPtr Gate::getSession(uint32_t serial)
{
    auto iter = sessions_.find(serial);
    if (iter != sessions_.end())
    {
        return iter->second;
    }
    return SessionPtr();
}

void Gate::kick(SessionPtr session)
{
    if (!session->closed_ && session->socket_.is_open())
    {
        session->socket_.shutdown(boost::asio::socket_base::shutdown_both);
        session->socket_.close();
        session->closed_ = true;
    }
}

bool Gate::kick(uint32_t serial)
{
    auto session = getSession(serial);
    if (session)
    {
        kick(session);
        return true;
    }
    return false;
}

void Gate::kickAll()
{
    sessions_.clear();
}

bool Gate::send(uint32_t serial, ByteRange data)
{
    auto session = getSession(serial);
    if (session)
    {
        return sessionWritePacket(session, data);
    }
    return false;
}

void Gate::sendAll(const void* buf, size_t size)
{
    assert(buf && size > 0);
    for (auto& item : sessions_)
    {
        auto& session = item.second;
        ByteRange data(reinterpret_cast<const uint8_t*>(buf), size);
        sessionWritePacket(session, data);
    }
}

uint32_t Gate::nextSerial()
{
    while (sessions_.count(current_serial_++))
        ;
    return current_serial_;
}

void Gate::startAccept()
{
    auto serial = nextSerial();
    SessionPtr session = std::make_shared<Session>(io_service_, serial);
    acceptor_.async_accept(session->socket_, std::bind(&Gate::handleAccept,
        this, _1, session));
}

void Gate::handleAccept(const boost::system::error_code& err, SessionPtr session)
{
    auto serial = session->serial_;
    if (err)
    {
        on_read_(err.value(), serial, StringPiece(err.message()));
        return ;
    }
    if (sessions_.size() < max_connections_)
    {
        auto endpoint = session->socket_.remote_endpoint();
        session->remote_addr_ = endpoint.address().to_string();
        const auto& address = session->remote_addr_;
        if (!black_list_.count(address))
        {
            sessions_[serial] = session;
            sessionStartRead(session);
        }
        else
        {
            auto errmsg = stringPrintf("address [%s] was in black list.", address.c_str());
            on_read_(ERR_ADDRRESS_BANNED, serial, StringPiece(errmsg));
        }
    }
    else
    {
        auto errmsg = stringPrintf("too many connections now, current is: %" PRIu64, 
            sessions_.size());
        on_read_(ERR_MAX_CONN_LIMIT, serial, StringPiece(errmsg));
    }
    if (acceptor_.is_open())
    {
        startAccept();
    }
}

void Gate::denyAddress(const std::string& address)
{
    black_list_.insert(address);
}

void Gate::allowAddress(const std::string& address)
{
    black_list_.erase(address);
}


void Gate::checkHeartBeat()
{
    std::vector<uint32_t> dead_connections;
    dead_connections.reserve(kDeadConnectionReserveSize);

    // garbage collecting
    for (auto& item : sessions_)
    {
        auto& session = item.second;
        if (session->closed_)
        {
            dead_connections.emplace_back(item.first);
        }
    }
    for (auto serial : dead_connections)
    {
        sessions_.erase(serial);
    }

    // heart-beat checking
    time_t now = time(NULL);
    for (auto& item : sessions_)
    {
        auto& session = item.second;
        auto elapsed = now - session->last_recv_time_;
        if (elapsed >= heart_beat_sec_)
        {
            kick(session);
            on_read_(ERR_HEARTBEAT_TIMEOUT, session->serial_, StringPiece("timeout"));
        }
    }

    // repeat timer
    drop_timer_.expires_from_now(std::chrono::seconds(heart_beat_check_sec_));
    drop_timer_.async_wait(std::bind(&Gate::checkHeartBeat, this));
}

void Gate::sessionStartRead(SessionPtr session)
{
    auto& head = session->head_;
    boost::asio::async_read(session->socket_, 
        boost::asio::buffer(&head, sizeof(head)),
        std::bind(&Gate::handleReadHead, this, session->serial_, _1, _2));
}

bool Gate::sessionWritePacket(SessionPtr session, ByteRange data)
{
    if (data.size() > 0 && data.size() < MAX_SEND_BYTES)
    {
        while (data.size() > MAX_PACKET_SIZE)
        {
            auto frame = data.subpiece(0, MAX_PACKET_SIZE);
            sessionWriteFrame(session, frame, true);
            data.advance(MAX_PACKET_SIZE);
        }
        sessionWriteFrame(session, data, 0);
        return true;
    }
    else
    {
        return false;
    }
}

void Gate::sessionWriteFrame(SessionPtr session, ByteRange frame, bool more)
{
    assert(frame.size() <= UINT16_MAX);
    auto out = compressServerPacket(frame, more);
    if (out && !out->empty())
    {
        boost::asio::async_write(session->socket_, 
            boost::asio::buffer(out->buffer(), out->length()), 
            std::bind(&Gate::handleWrite, this, session->serial_, _1, _2, out));
    }
}

void Gate::handleReadHead(uint32_t serial, const boost::system::error_code& ec, size_t bytes)
{
    auto session = getSession(serial);
    if (!session)
    {
        return;
    }
    if (ec)
    {
        kick(session);
        on_read_(ec.value(), serial, StringPiece(ec.message()));
        return;
    }

    auto& head = session->head_;
    if (head.size > MAX_PACKET_SIZE)
    {
        kick(session);
        auto errmsg = stringPrintf("invalid body size: %" PRIu64, bytes);
        on_read_(ERR_INVALID_BODY_SIZE, serial, StringPiece(errmsg));
        return;
    }
    if (head.size == 0) // empty content means heartbeating
    {
        session->last_recv_time_ = time(NULL);
        sessionStartRead(session); // ready for packet content body
    }
    else
    {
        auto& buffer = session->recv_buf_;
        if (buffer.size() < head.size)
        {
            buffer.resize(head.size);
        }
        boost::asio::async_read(session->socket_, 
            boost::asio::buffer(buffer.data(), head.size),
            std::bind(&Gate::handleReadBody, this, serial, _1, _2));
    }
}

void Gate::handleReadBody(uint32_t serial, const boost::system::error_code& ec, size_t bytes)
{
    auto session = getSession(serial);
    if (!session)
    {
        return;
    }
    if (ec)
    {
        kick(session);
        on_read_(ec.value(), serial, StringPiece(ec.message()));
        return;
    }

    auto& head = session->head_;
    auto& buffer = session->recv_buf_;
    const uint8_t* data = buffer.data();
    auto checksum = crc32c(data, bytes);
    if (head.checksum == checksum)
    {
        session->last_recv_time_ = time(NULL);
        auto out = uncompressPacketFrame((CodecType)head.codec, ByteRange(data, bytes));
        if (out)
        {
            on_read_(0, serial, out->byteRange());
        }
        sessionStartRead(session); // waiting for another packet
    }
    else
    {
        kick(session);
        auto errmsg = stringPrintf("invalid body checksum, expect %x, but get %x",
            checksum, head.checksum);
        on_read_(ERR_INVALID_CHECKSUM, serial, StringPiece(errmsg));
    }
}

void Gate::handleWrite(uint32_t serial, const boost::system::error_code& ec,
                       size_t bytes, std::shared_ptr<IOBuf> buf)
{
    if (ec)
    {
        on_read_(ec.value(), serial, StringPiece(ec.message()));
    }
}

} // namespace net
