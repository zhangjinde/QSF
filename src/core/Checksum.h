//  Copyright (c) 2013, Facebook, Inc.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.

#pragma once

#include <stdint.h>
#include <stddef.h>


/**
 * Compute the CRC-32C checksum of a buffer, using a hardware-accelerated
 * implementation if available or a portable software implementation as
 * a default.
 *
 * @note CRC-32C is different from CRC-32; CRC-32C starts with a different
 *       polynomial and thus yields different results for the same input
 *       than a traditional CRC-32.
 */

#ifdef __cplusplus
extern "C"
#endif
uint32_t crc32c(const void* data, size_t nbytes, uint32_t init_crc);
