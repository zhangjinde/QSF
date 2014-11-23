#pragma once

#include "iobuf.h"
#include "packet.h"

namespace net {

/**
 * Compression and decompression over I/O buffers
 */
std::shared_ptr<IOBuf> compressServerPacket(ByteRange frame, bool more);
std::shared_ptr<IOBuf> compressClientPacket(ByteRange frame);
std::shared_ptr<IOBuf> uncompressPacketFrame(CodecType codec, ByteRange frame);

} // namespace net
