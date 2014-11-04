#include "compression.h"
#include "core/conv.h"
#include "core/logging.h"
#include "core/scope_guard.h"
#include <zlib.h>

namespace {

static void FreeFn(void* data) {}

inline std::unique_ptr<IOBuf> ZlibCompress(ByteRange data, size_t reserved)
{
    z_stream stream;
    memset(&stream, 0, sizeof(stream));

    SCOPE_EXIT
    {
        int rc = inflateEnd(&stream);
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

inline std::unique_ptr<IOBuf> ZlibUnCompress(ByteRange data)
{
    static THREAD_LOCAL uint8_t buffer[MAX_PACKET_SIZE*2];

    z_stream stream;
    memset(&stream, 0, sizeof(stream));

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
    stream.next_out = buffer;
    stream.avail_out = sizeof(buffer);

    rc = inflate(&stream, Z_FINISH);
    CHECK_EQ(rc, Z_STREAM_END) << stream.msg;

    return IOBuf::takeOwnership(buffer, stream.total_out, FreeFn);
}

} // anounymouse namespace

std::unique_ptr<IOBuf> Compress(CodecType codec, ByteRange data, size_t reserved)
{
    switch (codec)
    {
    case ZLIB:
        return ZlibCompress(data, reserved);
    default:
        throw std::invalid_argument(to<std::string>(
            "Compression type ", codec, " not supported"));
    }
}

std::unique_ptr<IOBuf> UnCompress(CodecType codec, ByteRange data)
{
    switch (codec)
    {
    case ZLIB:
        return ZlibUnCompress(data);
    default:
        throw std::invalid_argument(to<std::string>(
            "Compression type ", codec, " not supported"));
    }
}
