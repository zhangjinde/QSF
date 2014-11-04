#pragma once

#include "core/range.h"
#include "packet.h"
#include "iobuf.h"

/**
 * Compression and decompression over I/O buffers
 */
std::unique_ptr<IOBuf> Compress(CodecType codec, ByteRange data, size_t reserved = 0U);
std::unique_ptr<IOBuf> UnCompress(CodecType codec, ByteRange data);
