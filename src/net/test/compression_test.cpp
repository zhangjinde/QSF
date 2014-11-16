#include <gtest/gtest.h>
#include <string>
#include <random>
#include "net/compression.h"

using namespace std;


static std::random_device rand_device;
static std::default_random_engine prng(rand_device());

string readableString(size_t size)
{
    static const char alpha[] = 
        "0123456789"
        "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    const size_t length = strlen(alpha);

    string s;
    s.resize(size);
    for (size_t i = 0; i < size; i++)
    {
        auto chosen = std::uniform_int_distribution<uint32_t>(0, length-1)(prng);
        s[i] = alpha[chosen];
    }
    return s;
}

TEST(compression, zlibCompress)
{
    for (auto size : { 32, 64, 128, 512, 1024, 2048, 8196 })
    {
        auto str = readableString(size);
        ByteRange origin(reinterpret_cast<const uint8_t*>(str.data()), str.length());
        auto buf = net::compress(net::ZLIB, origin);
        EXPECT_FALSE(buf->empty());
        auto compressed = buf->byteRange();
        auto buf_out = net::uncompress(net::ZLIB, compressed);
        auto uncompressed = buf_out->byteRange();
        EXPECT_EQ(origin, uncompressed);
    }
}
