// Copyright (C) 2014 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#pragma once

#include <cstdint>

/**
 *  a network packet on the wire consist of :
 *
 *  +---------------+----------------------+
 *  |    Header     |     content          |
 *  +---------------+----------------------+
 *
 */

namespace net {

enum
{
    // max content size, size of a packet is limited to 16K
    MAX_PACKET_SIZE = 16 * 1024,

    // max 512K
    MAX_SEND_BYTES = 32 * MAX_PACKET_SIZE,
};

const int kRecvBufReserveSize = 1024;

// Compression / decompression
enum CodecType
{
    // no compression
    NO_COMPRESSION = 0,

    // Use LZ4 compression.
    LZ4 = 1,

    // Use Snappy compression
    SNAPPY = 2,

    // Use zlib compression
    ZLIB = 3,

    // Use LZMA2 compression.
    LZMA2 = 4,
};


#pragma pack(push) 
#pragma pack(2)

struct ClientHeader
{
    uint16_t    size;       // body size
    uint16_t    checksum;   // checksum value of content
};

struct ServerHeader
{
    uint16_t    size;       // body size
    uint8_t     codec;      // compression type
    uint8_t     more;       // more data
};

#pragma pack(pop)

} // namespace net
