#include "compression.h"
#include "core/conv.h"
#include "core/logging.h"
#include "core/scope_guard.h"
#include "core/checksum.h"
#include <zlib.h>

namespace net {
namespace {

inline std::shared_ptr<IOBuf> zlibCompress(ByteRange data, size_t reserved)
{
    z_stream stream;
    stream.zalloc = nullptr;
    stream.zfree = nullptr;
    stream.opaque = nullptr;

    SCOPE_EXIT
    {
        int rc = deflateEnd(&stream);
        CHECK(rc == Z_OK) << stream.msg;
    };

    int rc = deflateInit(&stream, Z_DEFAULT_COMPRESSION);
    if (rc != Z_OK)
    {
        throw std::runtime_error(to<std::string>(
            "ZlibCompress: deflateInit error: ", rc, ": ", stream.msg));
    }
    uint32_t maxCompressedLength = deflateBound(&stream, data.size());
    auto out = IOBuf::create(maxCompressedLength + reserved);
    out->append(reserved);
    stream.next_in = const_cast<uint8_t*>(data.data());
    stream.avail_in = static_cast<uint32_t>(data.size());
    stream.next_out = out->data();
    stream.avail_out = maxCompressedLength;

    rc = deflate(&stream, Z_FINISH);
    CHECK_EQ(rc, Z_STREAM_END) << stream.msg;
    out->append(stream.total_out);
    return out;
}

inline std::shared_ptr<IOBuf> zlibUnCompress(ByteRange data)
{
    static THREAD_LOCAL uint8_t uncompBuffer[MAX_PACKET_SIZE*2];

    z_stream stream;
    stream.zalloc = nullptr;
    stream.zfree = nullptr;
    stream.opaque = nullptr;

    SCOPE_EXIT
    {
        int rc = inflateEnd(&stream);
        CHECK(rc == Z_OK) << stream.msg;
    };

    int rc = inflateInit(&stream);
    if (rc != Z_OK)
    {
        throw std::runtime_error(to<std::string>(
            "ZlibCodec: inflateInit error: ", rc, ": ", stream.msg));
    }
    stream.next_in = const_cast<uint8_t*>(data.data());
    stream.avail_in = static_cast<uint32_t>(data.size());
    stream.next_out = uncompBuffer;
    stream.avail_out = sizeof(uncompBuffer);

    rc = inflate(&stream, Z_FINISH);
    CHECK_EQ(rc, Z_STREAM_END) << stream.msg;

    return IOBuf::takeOwnership(uncompBuffer, stream.total_out, [](void*){});
}

} // anounymouse namespace


std::shared_ptr<IOBuf> compressServerPacket(CodecType codec, ByteRange frame, bool more)
{
    assert(frame.size() > 0 && frame.size() <= UINT16_MAX);
    const auto head_size = sizeof(ServerHeader);
    switch (codec)
    {
    case NO_COMPRESSION:
        {
            auto out = IOBuf::create(head_size + frame.size());
            memcpy(out->buffer() + head_size, frame.data(), frame.size());
            out->append(head_size + frame.size());
            ServerHeader* head = reinterpret_cast<ServerHeader*>(out->buffer());
            head->size = static_cast<uint16_t>(out->length() - head_size);
            head->codec = NO_COMPRESSION;
            head->more = more;
            return out;
        }
    case ZLIB:
        {
            auto out = zlibCompress(frame, head_size);
            ServerHeader* head = reinterpret_cast<ServerHeader*>(out->buffer());
            head->size = static_cast<uint16_t>(out->length() - head_size);
            head->codec = ZLIB;
            head->more = more;
            return out;
        }
    default:
        throw std::invalid_argument(to<std::string>(
            "Compression type ", codec, " not supported"));
    }
}

std::shared_ptr<IOBuf> compressClientPacket(CodecType codec, ByteRange frame)
{
    assert(frame.size() > 0 && frame.size() <= UINT16_MAX);
    const auto head_size = sizeof(ClientHeader);
    auto out = IOBuf::create(head_size + frame.size());
    memcpy(out->buffer() + head_size, frame.data(), frame.size());
    out->append(head_size + frame.size());
    ClientHeader* head = reinterpret_cast<ClientHeader*>(out->buffer());
    head->size = static_cast<uint16_t>(out->length() - head_size);
    head->checksum = crc16(out->buffer() + head_size, frame.size());
    return out;
}

std::shared_ptr<IOBuf> uncompressPacketFrame(CodecType codec, ByteRange frame)
{
    assert(frame.size() <= UINT16_MAX);
    switch (codec)
    {
    case NO_COMPRESSION:
        {
            uint8_t* data = const_cast<uint8_t*>(frame.data());
            return IOBuf::takeOwnership((void*)data, frame.size(), [](void*){});
        }
    case ZLIB:
        return zlibUnCompress(frame);
    default:
        throw std::invalid_argument(to<std::string>(
            "Compression type ", codec, " not supported"));
    }
}

} // namespace net
