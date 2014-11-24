#include "gate.h"
#include <ctime>
#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>
#include "core/logging.h"
#include "session-inl.h"

namespace net {

#ifdef NDEBUG
const int kCheckHeartBeatSec = 5;
#else
const int kCheckHeartBeatSec = 90;
#endif

const int kDeadConnectionReserveSize = 8;

Gate::Gate(boost::asio::io_service& io_service, 
           uint32_t max_connections, 
           uint32_t heart_beat_sec)
    : io_service_(io_service), 
      acceptor_(io_service), 
      drop_timer_(io_service, std::chrono::seconds(kCheckHeartBeatSec)),
      max_connections_(max_connections),
      heart_beat_sec_(heart_beat_sec)
{
}

Gate::~Gate()
{
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

bool Gate::kick(uint32_t serial)
{
    auto iter = sessions_.find(serial);
    if (iter != sessions_.end())
    {
        auto session = iter->second;
        session->close();
        return true;
    }
    return false;
}

void Gate::kickAll()
{
    for (auto& item : sessions_)
    {
        item.second->close();
    }
}

void Gate::send(uint32_t serial, ByteRange data)
{
    auto iter = sessions_.find(serial);
    if (iter != sessions_.end())
    {
        auto& session = iter->second;
        session->write(data);
    }
}

void Gate::sendAll(const void* data, size_t size)
{
    assert(data && size > 0);
    for (auto& item : sessions_)
    {
        auto& session = item.second;
        session->write(ByteRange(reinterpret_cast<const uint8_t*>(data), size));
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
    SessionPtr session = std::make_shared<Session>(io_service_, serial, on_read_);
    acceptor_.async_accept(session->socket(), std::bind(&Gate::handleAccept,
        this, _1, session));
}

void Gate::handleAccept(const boost::system::error_code& err, SessionPtr ptr)
{
    if (acceptor_.is_open())
    {
        startAccept();
    }
    if (err)
    {
        LOG(INFO) << err.message();
        return ;
    }
    if (sessions_.size() < max_connections_)
    {
        const auto& address = ptr->remoteAddress();
        if (!black_list_.count(address))
        {
            sessions_[ptr->serial()] = ptr;
            ptr->startRead();
        }
        else
        {
            LOG(DEBUG) << "session banned: " << address;
        }
    }
    else
    {
        LOG(DEBUG) << "max session limits: " << max_connections_;
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
    time_t now = time(NULL);
    for (auto& item : sessions_)
    {
        auto& session = item.second;
        auto elapsed = now - session->lastRecvTime();
        if (elapsed >= heart_beat_sec_)
        {
            session->close(boost::asio::error::timed_out);
        }
        if (session->isClosed())
        {
            dead_connections.emplace_back(item.first);
        }
    }
    for (auto serial : dead_connections)
    {
        sessions_.erase(serial);
    }

    // repeat timer
    drop_timer_.expires_from_now(std::chrono::seconds(kCheckHeartBeatSec));
    drop_timer_.async_wait(std::bind(&Gate::checkHeartBeat, this));
}

} // namespace net
