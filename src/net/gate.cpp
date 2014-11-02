#include "gate.h"
#include <ctime>
#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>
#include "core/logging.h"
#include "session-inl.h"

#ifdef NDEBUG
const int kCheckHeartBeatSec = 10;
#else
const int kCheckHeartBeatSec = 30;
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

void Gate::Start(const std::string& host, uint16_t port, ReadCallback callback)
{
    using namespace boost::asio::ip;
    assert(callback);
    on_read_ = callback;

    tcp::endpoint endpoint(address::from_string(host), port);
    acceptor_.open(endpoint.protocol());
    acceptor_.set_option(tcp::acceptor::reuse_address(true));
    acceptor_.bind(endpoint);
    acceptor_.listen();

    drop_timer_.async_wait(std::bind(&Gate::CheckHeartBeat, this));
    StartAccept();
}

bool Gate::Kick(uint32_t serial)
{
    auto iter = sessions_.find(serial);
    if (iter != sessions_.end())
    {
        auto session = iter->second;
        session->Close();
        return true;
    }
    return false;
}

void Gate::Send(uint32_t serial, ByteRange data)
{
    auto iter = sessions_.find(serial);
    if (iter != sessions_.end())
    {
        auto session = iter->second;
        session->AsynWrite(data);
    }
}

uint32_t Gate::NextSessionSerial()
{
    while (sessions_.count(current_serial_++))
        ;
    return current_serial_;
}

void Gate::StartAccept()
{
    auto serial = NextSessionSerial();
    SessionPtr session = std::make_shared<Session>(io_service_, serial, on_read_);
    acceptor_.async_accept(session->GetSocket(), std::bind(&Gate::HandleAccept,
        this, _1, session));
}

void Gate::HandleAccept(const boost::system::error_code& err, SessionPtr ptr)
{
    if (!err)
    {
        if (sessions_.size() < max_connections_)
        {
            const auto& address = ptr->GetAddress();
            if (!black_list_.count(address))
            {
                sessions_[ptr->GetSerial()] = ptr;
                ptr->AsynRead();
            }
            else
            {
                LOG(INFO) << "session banned: " << address;
            }
        }
        else
        {
            LOG(INFO) << "reach max session limits: " << max_connections_;
        }
    }
    if (acceptor_.is_open())
    {
        StartAccept();
    }
}

void Gate::DenyAddress(const std::string& address)
{
    black_list_.insert(address);
}

void Gate::AllowAddress(const std::string& address)
{
    black_list_.erase(address);
}

void Gate::CheckHeartBeat()
{
    std::vector<uint32_t> dead_connections;
    dead_connections.reserve(kDeadConnectionReserveSize);
    time_t now = time(NULL);
    for (auto& item : sessions_)
    {
        auto& session = item.second;
        auto elapsed = now - session->GetLastRecvTime();
        if (elapsed >= heart_beat_sec_)
        {
            session->Close(boost::asio::error::timed_out);
        }
        if (session->IsClosed())
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
    drop_timer_.async_wait(std::bind(&Gate::CheckHeartBeat, this));
}
