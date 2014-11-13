#pragma once

#include "core/range.h"
#include "packet.h"
#include "iobuf.h"

namespace net {

/**
 * Compression and decompression over I/O buffers
 */
std::unique_ptr<IOBuf> compress(CodecType codec, ByteRange data, size_t reserved = 0U);
std::unique_ptr<IOBuf> uncompress(CodecType codec, ByteRange data);

} // namespace net
