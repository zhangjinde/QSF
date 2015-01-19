// Copyright (C) 2014 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#pragma once

#include "IOBuf.h"
#include "Packet.h"

namespace net {

/**
 * Compression and decompression over I/O buffers
 */
std::shared_ptr<IOBuf> compressServerPacket(CodecType codec, ByteRange frame, uint8_t key, uint8_t more);

std::shared_ptr<IOBuf> compressClientPacket(CodecType codec, ByteRange frame, uint8_t key);

std::shared_ptr<IOBuf> uncompressPacketFrame(CodecType codec, ByteRange frame, uint8_t key);

} // namespace net
