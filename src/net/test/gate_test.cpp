#include <gtest/gtest.h>
#include <string>
#include "rand_string.h"
#include "net/gate.h"
#include "net/client.h"

using namespace std;

const static string DEFAULT_HOST = "127.0.0.1";
const static uint16_t DEFAULT_PORT = 10086;


TEST(Gate, start)
{
    boost::asio::io_service io_service;
    net::Gate gate(io_service);
    gate.start(DEFAULT_HOST, DEFAULT_PORT, 
        [&](int err, uint32_t serial, ByteRange data)
    {
    });

    io_service.poll();

    EXPECT_EQ(gate.size(), 0);
    gate.stop();
}

TEST(Gate, connect)
{
    string msg = "hello";

    boost::asio::io_service io_service;
    net::Gate gate(io_service);
    gate.start(DEFAULT_HOST, DEFAULT_PORT,
        [&](int err, uint32_t serial, ByteRange data)
    {
        if (!err)
        {
            EXPECT_EQ(msg, StringPiece(data));
            io_service.stop();
        }
    });

    net::Client client(io_service);
    client.connect(DEFAULT_HOST, DEFAULT_PORT,
        [&](const boost::system::error_code& ec)
    {
        EXPECT_TRUE(!ec);
    });

    net::Client client2(io_service);
    EXPECT_NO_THROW(client2.connect(DEFAULT_HOST, DEFAULT_PORT));
    client2.send(msg);

    io_service.run();
}

TEST(Gate, denyAddress)
{
    string msg = "hello";
    boost::asio::io_service io_service;
    net::Gate gate(io_service);
    gate.start(DEFAULT_HOST, DEFAULT_PORT,
        [&](int err, uint32_t serial, ByteRange data)
    {
    });

    gate.denyAddress("127.0.0.1");

    net::Client client(io_service);
    client.connect(DEFAULT_HOST, DEFAULT_PORT,
        [&](const boost::system::error_code& ec)
    {
        if (!ec)
        {
            client.send(msg);
            io_service.stop();
        }
    });

    io_service.run();

    EXPECT_EQ(gate.size(), 0);
    gate.kickAll();
}

void test_send(const string& msg)
{
    ByteRange bytes = StringPiece(msg);
    boost::asio::io_service io_service;
    net::Gate gate(io_service);
    gate.start(DEFAULT_HOST, DEFAULT_PORT,
        [&](int err, uint32_t serial, ByteRange data)
    {
        if (!err)
        {
            EXPECT_EQ(bytes, data);
            gate.send(serial, data);
        }
    });


    net::Client client(io_service);
    client.connect(DEFAULT_HOST, DEFAULT_PORT,
        [&](const boost::system::error_code& ec)
    {
        EXPECT_TRUE(!ec);
        if (!ec)
        {
            client.send(msg);
            client.startRead([&](ByteRange data)
            {
                EXPECT_EQ(bytes, data);
                io_service.stop();
            });
        }
    });

    io_service.run();
}

TEST(Gate, send)
{
    for (auto size : { 32, 64, 128, 512, 1024, 2048, 8196 })
    {
        auto msg = randString(size);
        test_send(msg);
    }
}