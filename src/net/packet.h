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

enum
{
    // max content size, size of a packet is limited to 64k
    MAX_PACKET_SIZE = UINT16_MAX,
};

struct Header
{
    uint16_t     size;          // content size
    uint8_t      codec;         // compression type
    uint8_t      more;          // more data
    uint32_t     checksum;      // crc32c checksum value of content
};
