#include "gate.h"
#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>
#include "core/logging.h"
#include "packet.h"
#include "checksum.h"

using namespace std::placeholders;

class Gate::Session
{
public:
    Session(boost::asio::io_service& io_service,
            uint32_t serial,
            ReadCallback& callback)
        : socket_(io_service), serial_(serial), on_read_(callback)
    {
        CHECK(callback);
    }

    ~Session()
    {
    }

    void StartRead()
    {
        boost::asio::async_read(socket_, boost::asio::buffer(&head_, sizeof(head_)),
            std::bind(&Session::HandleReadHead, this, _1, _2));
    }

    void AsynWrite(ByteRange data)
    {
        Header head = {};
        head.size = static_cast<uint16_t>(data.size());
        size_t size = data.size() + sizeof(head);
        uint8_t* buffer = (uint8_t*)malloc(size);
        if (buffer == nullptr)
        {
            return;
        }
        memcpy(buffer, &head, sizeof(head));
        memcpy(buffer_+sizeof(head), data.data(), data.size());
        boost::asio::async_write(socket_, boost::asio::buffer(buffer, size),
            std::bind(&Session::HandleWrite, this, _1, _2, buffer));
    }

    uint32_t GetSerial() const { return serial_; }

    boost::asio::ip::tcp::socket& GetSocket() { return socket_; }

private:
    void HandleReadHead(const boost::system::error_code& err, size_t bytes)
    {
        boost::asio::async_read(socket_, boost::asio::buffer(buffer_, head_.size),
            std::bind(&Session::HandleReadBody, this, _1, _2));
    }

    void HandleReadBody(const boost::system::error_code& err, size_t bytes)
    {
        on_read_(serial_, ByteRange(buffer_, bytes));
        StartRead();
    }

    void HandleWrite(const boost::system::error_code& err, size_t bytes, uint8_t* buf)
    {
        free(buf);
    }

private:
    boost::asio::ip::tcp::socket    socket_;
    uint32_t serial_;

    Header  head_;

    uint8_t buffer_[MAX_PACKET_SIZE - sizeof(Header)];

    ReadCallback&   on_read_;
};


//////////////////////////////////////////////////////////////////////////
Gate::Gate(boost::asio::io_service& io_service)
    : io_service_(io_service), acceptor_(io_service), drop_timer_(io_service)
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

    StartAccept();
}

void Gate::StartAccept()
{
    while (sessions_.count(current_serial_++))
        ;
    SessionPtr session = std::make_shared<Session>(io_service_, 
        current_serial_, on_read_);
    acceptor_.async_accept(session->GetSocket(), std::bind(&Gate::HandleAccept,
        this, _1, session));
}


void Gate::HandleAccept(const boost::system::error_code& err, SessionPtr ptr)
{
    if (!err)
    {
        sessions_[ptr->GetSerial()] = ptr;
        ptr->StartRead();
    }
    if (acceptor_.is_open())
    {
        StartAccept();
    }
}