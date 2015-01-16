#include <gtest/gtest.h>
#include <cinttypes>
#include <string>
#include "RandString.h"
#include "net/Gate.h"
#include "net/Client.h"
#include "core/Strings.h"

using namespace std;
using namespace net;

const static string DEFAULT_HOST = "127.0.0.1";
const static uint16_t DEFAULT_PORT = 10086;


TEST(Gate, start)
{
    asio::io_service io_service;
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

    asio::io_service io_service;
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
        [&](const std::error_code& ec)
    {
        EXPECT_TRUE(!ec);
    });

    net::Client client2(io_service);
    EXPECT_NO_THROW(client2.connect(DEFAULT_HOST, DEFAULT_PORT));
    client2.write(msg);

    io_service.run();
}

TEST(Gate, denyAddress)
{
    string msg = "hello";
    asio::io_service io_service;
    net::Gate gate(io_service);
    gate.start(DEFAULT_HOST, DEFAULT_PORT,
        [&](int err, uint32_t serial, ByteRange data)
    {
    });

    gate.denyAddress("127.0.0.1");

    net::Client client(io_service);
    client.connect(DEFAULT_HOST, DEFAULT_PORT,
        [&](const std::error_code& ec)
    {
        if (!ec)
        {
            client.write(msg);
            io_service.stop();
        }
    });

    io_service.run();

    EXPECT_EQ(gate.size(), 0);
    gate.kickAll();
}

static void test_send(const string& msg)
{
    ByteRange bytes = StringPiece(msg);
    std::string request = stringPrintf("request msg size %" PRIu64, msg.size());

    asio::io_service io_service;
    net::Gate gate(io_service);
    gate.start(DEFAULT_HOST, DEFAULT_PORT,
        [&](int err, uint32_t serial, ByteRange data)
    {
        if (!err)
        {
            ByteRange r = StringPiece(request);
            EXPECT_EQ(r, data);
            gate.send(serial, bytes);
        }
    });

    net::Client client(io_service);
    client.connect(DEFAULT_HOST, DEFAULT_PORT,
        [&](const std::error_code& ec)
    {
        EXPECT_TRUE(!ec);
        if (!ec)
        {
            client.write(request);
            client.startRead([&](ByteRange response)
            {
                EXPECT_EQ(bytes, response);
                io_service.stop();
            });
        }
    });

    io_service.run();
}

TEST(Gate, send)
{
    int sizeTable[] = { 32, 128, 1024, 8196, MAX_PACKET_SIZE+1, MAX_PACKET_SIZE*4 };
    for (auto size : sizeTable)
    {
        auto msg = randString(size);
        test_send(msg);
    }
}