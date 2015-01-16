#pragma once

#include "IOBuf.h"
#include "Packet.h"

namespace net {

/**
 * Compression and decompression over I/O buffers
 */
std::shared_ptr<IOBuf> compressServerPacket(CodecType codec, ByteRange frame, bool more);
std::shared_ptr<IOBuf> compressClientPacket(CodecType codec, ByteRange frame);
std::shared_ptr<IOBuf> uncompressPacketFrame(CodecType codec, ByteRange frame);

} // namespace net
