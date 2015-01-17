#include <gtest/gtest.h>
#include <string>
#include <random>
#include "RandString.h"
#include "core/Random.h"
#include "net/Compression.h"

using namespace std;
using namespace net;

TEST(compression, compressServerPacket)
{
    for (auto size : { 32, 64, 128, 512, 1024, 8196 })
    {
        auto str = randString(size);
        uint8_t key = static_cast<uint8_t>(Random::rand32(0xff));
        ByteRange origin = StringPiece(str);
        CodecType codec = (size >= 128 ? ZLIB : NO_COMPRESSION);
        auto buf = compressServerPacket(codec, origin, key, PACKET_FRAME_MSG);
        EXPECT_FALSE(buf->empty());
        const ServerHeader* head = reinterpret_cast<const ServerHeader*>(buf->buffer());
        const size_t head_size = sizeof(*head);
        for (int i = 0; i < head_size; ++i)
        {
            auto p = ((uint8_t*)head) + i;
            *p = *p ^ key;
        }
        EXPECT_EQ(head->codec, codec);
        auto compressed_frame = ByteRange(buf->buffer() + head_size, buf->length()-head_size);
        auto buf_out = uncompressPacketFrame((CodecType)head->codec, compressed_frame, key);
        EXPECT_EQ(origin, buf_out->byteRange());
    }
}

TEST(compression, compressClientPacket)
{
    for (auto size : { 32, 64, 128, 512, 1024, 8196 })
    {
        auto str = randString(size);
        uint8_t key = static_cast<uint8_t>(Random::rand32(0xff));
        const CodecType codec = NO_COMPRESSION;
        ByteRange origin = StringPiece(str);
        auto buf = compressClientPacket(codec, origin, key);
        EXPECT_FALSE(buf->empty());
        const ClientHeader* head = reinterpret_cast<const ClientHeader*>(buf->buffer());
        const size_t head_size = sizeof(*head);
        for (int i = 0; i < head_size; ++i)
        {
            auto p = ((uint8_t*)head) + i;
            *p = *p ^ key;
        }
        auto compressed_frame = ByteRange(buf->buffer() + head_size, buf->length() - head_size);
        auto buf_out = uncompressPacketFrame(codec, compressed_frame, key);
        EXPECT_EQ(origin, buf_out->byteRange());
    }
}
