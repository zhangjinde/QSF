#include <gtest/gtest.h>
#include <string>
#include <random>
#include "RandString.h"
#include "net/Compression.h"

using namespace std;
using namespace net;

TEST(compression, compressServerPacket)
{
    for (auto size : { 32, 64, 128, 512, 1024, 2048, 8196 })
    {
        auto str = randString(size);
        ByteRange origin = StringPiece(str);
        CodecType codec = size > 128 ? ZLIB : NO_COMPRESSION;
        auto buf = compressServerPacket(codec, origin, false);
        EXPECT_FALSE(buf->empty());
        const ServerHeader* head = reinterpret_cast<const ServerHeader*>(buf->buffer());
        const size_t head_size = sizeof(*head);
        EXPECT_EQ(head->codec, codec);
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
        const CodecType codec = NO_COMPRESSION;
        ByteRange origin = StringPiece(str);
        auto buf = compressClientPacket(codec, origin);
        EXPECT_FALSE(buf->empty());
        const ClientHeader* head = reinterpret_cast<const ClientHeader*>(buf->buffer());
        const size_t head_size = sizeof(*head);
        auto compressed_frame = ByteRange(buf->buffer() + head_size, buf->length() - head_size);
        auto buf_out = uncompressPacketFrame(codec, compressed_frame);
        EXPECT_EQ(origin, buf_out->byteRange());
    }
}
