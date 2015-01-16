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
    // max content size, size of a packet is limited to 8K
    MAX_PACKET_SIZE = 8 * 1024,

    // max 128K
    MAX_SEND_BYTES = 128 * 1024,

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

// Compression / decompression
enum CodecType
{
    // no compression
    NO_COMPRESSION = 0,

    // Use zlib compression
    ZLIB = 1,
};


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

} // namespace net
