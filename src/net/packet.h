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

    DEFAULT_NO_COMPRESSION_SIZE = 128,

    // default value of max client connection
    DEFAULT_MAX_CONNECTIONS = 6000,

    // default value of heartbeat seconds
#ifdef NDEBUG
    DEFAULT_MAX_HEARTBEAT_SEC = 10,
    DEFAULT_HEARTBEAT_CHECK_SEC = 10,
#else
    DEFAULT_MAX_HEARTBEAT_SEC = 300,
    DEFAULT_HEARTBEAT_CHECK_SEC = 10,
#endif
};

// Protocol error code
enum
{
    ERR_ACCEPT_FAILED = 100001,        // accept failed
    ERR_ADDRRESS_BANNED = 100002,      // banned IP address
    ERR_MAX_CONN_LIMIT = 100003,       // reach max connection limit
    ERR_INVALID_BODY_SIZE = 100004,    // body size too big
    ERR_INVALID_CHECKSUM = 100005,     // invalid body checksum
    ERR_SND_SIZE_TOO_BIG = 100006,     // send buffer too big
    ERR_HEARTBEAT_TIMEOUT = 100007,    // timeout issue
};

const int kRecvBufReserveSize = 256;

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
#pragma pack(4)

struct ClientHeader
{
    uint16_t    size;       // body size
    uint8_t     codec;      // compression type
    uint8_t     flag;       // reserved
    uint32_t    checksum;   // checksum value of content
};


struct ServerHeader
{
    uint16_t    size;       // body size
    uint8_t     codec;      // compression type
    uint8_t     more;       // more data
};

static_assert(sizeof(ClientHeader) % 4 == 0, "must be 4 bytes packed");
static_assert(sizeof(ServerHeader) % 4 == 0, "must be 4 bytes packed");

#pragma pack(pop)

} // namespace net
