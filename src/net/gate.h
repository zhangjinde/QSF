#pragma once

#include <cstdint>
#include <ctime>
#include <string>
#include <memory>
#include <unordered_map>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/system/error_code.hpp>
#include <boost/noncopyable.hpp>
#include "core/range.h"

typedef std::function<void(uint32_t, ByteRange)>   ReadCallback;

class Gate : boost::noncopyable
{
    class Session;
    typedef std::shared_ptr<Session> SessionPtr;

public:
    explicit Gate(boost::asio::io_service& io_service);
    ~Gate();

    void Start(const std::string& host, uint16_t port, ReadCallback callback);

    void Stop();

    void Send(uint32_t serial, ByteRange data);
    void Kick(uint32_t serial);

private:
    void StartAccept();
    void HandleAccept(const boost::system::error_code& err, SessionPtr ptr);

private:
    boost::asio::io_service&        io_service_;
    boost::asio::ip::tcp::acceptor  acceptor_;

    boost::asio::steady_timer   drop_timer_;

    ReadCallback    on_read_;
    
    uint32_t current_serial_ = 10000;
    std::unordered_map<uint32_t, SessionPtr> sessions_;
};