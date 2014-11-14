#pragma once

#include "packet.h"
#include "iobuf.h"

namespace net {

/**
 * Compression and decompression over I/O buffers
 */
std::shared_ptr<IOBuf> compress(CodecType codec, ByteRange data, size_t reserved = 0U);
std::shared_ptr<IOBuf> uncompress(CodecType codec, ByteRange data);

} // namespace net
