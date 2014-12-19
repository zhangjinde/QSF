#include <gtest/gtest.h>
#include <string>
#include <random>
#include "rand_string.h"
#include "net/compression.h"

using namespace std;
using namespace net;

TEST(compression, compressServerPacket)
{
    for (auto size : { 32, 64, 128, 512, 1024, 2048, 8196 })
    {
        auto str = randString(size);
        ByteRange origin = StringPiece(str);
        auto buf = compressServerPacket(NO_COMPRESSION, origin, false);
        EXPECT_FALSE(buf->empty());
        const ServerHeader* head = reinterpret_cast<const ServerHeader*>(buf->buffer());
        const size_t head_size = sizeof(*head);
        EXPECT_EQ(head->size, origin.size());
        auto compressed_frame = ByteRange(buf->buffer() + head_size, buf->length()-head_size);
        auto buf_out = uncompressPacketFrame((CodecType)head->codec, compressed_frame);
        EXPECT_EQ(origin, buf_out->byteRange());
    }
}

TEST(compression, compressClientPacket)
{
    for (auto size : { 32, 64, 128, 512, 1024, 2048, 8196 })
    {
        auto str = randString(size);
        ByteRange origin = StringPiece(str);
        net::CodecType codec = NO_COMPRESSION;
        if (origin.size() > DEFAULT_NO_COMPRESSION_SIZE)
            codec = ZLIB;
        auto buf = compressClientPacket(codec, origin);
        EXPECT_FALSE(buf->empty());
        const ClientHeader* head = reinterpret_cast<const ClientHeader*>(buf->buffer());
        const size_t head_size = sizeof(*head);
        if (size <= DEFAULT_NO_COMPRESSION_SIZE)
        {
            EXPECT_EQ(head->codec, NO_COMPRESSION);
            EXPECT_EQ(head->size, origin.size());
        }
        else
        {
            EXPECT_EQ(head->codec, ZLIB);
        }
        auto compressed_frame = ByteRange(buf->buffer() + head_size, buf->length() - head_size);
        auto buf_out = uncompressPacketFrame((CodecType)head->codec, compressed_frame);
        EXPECT_EQ(origin, buf_out->byteRange());
    }
}
