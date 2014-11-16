#include <gtest/gtest.h>
#include <string>
#include <random>
#include "rand_string.h"
#include "net/compression.h"

using namespace std;


TEST(compression, zlibCompress)
{
    for (auto size : { 32, 64, 128, 512, 1024, 2048, 8196 })
    {
        auto str = randString(size);
        ByteRange origin(reinterpret_cast<const uint8_t*>(str.data()), str.length());
        auto buf = net::compress(net::ZLIB, origin);
        EXPECT_FALSE(buf->empty());
        auto compressed = buf->byteRange();
        auto buf_out = net::uncompress(net::ZLIB, compressed);
        auto uncompressed = buf_out->byteRange();
        EXPECT_EQ(origin, uncompressed);
    }
}
