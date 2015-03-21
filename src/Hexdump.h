// Copyright (C) 2014 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#pragma once

#include <assert.h>
#include <stdint.h>
#include <string>
#include "core/Range.h"

inline std::string  BinaryToHex(const void* data, size_t len)
{
    assert(data && len > 0);
    static const char dict[] = "0123456789abcdef";
    std::string result;
    result.reserve(len * 2);
    const uint8_t* buf = reinterpret_cast<const uint8_t*>(data);
    for (size_t i = 0; i < len; i++)
    {
        uint8_t ch = buf[i];
        result.push_back(dict[(ch & 0xF0) >> 4]);
        result.push_back(dict[ch & 0x0F]);
    }
    return std::move(result);
}

inline std::string BinaryToHex(ByteRange r)
{
    return BinaryToHex(r.data(), r.size());
}

inline std::string BinaryToHex(StringPiece s)
{
    return BinaryToHex(s.data(), s.size());
}