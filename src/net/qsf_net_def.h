// Copyright (C) 2014-2015 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#pragma once

/**
 *  an network packet on the wire consist of :
 *
 *  +---------------+----------------------+
 *  |    Header     |     content          |
 *  +---------------+----------------------+
 *
 */
 
// max net packet size
#define NET_MAX_PACKET_SIZE (1024*8)

// max alive connections per server
#define NET_DEFAULT_MAX_CONN    5000

// default heart beat seconds
#define NET_DEFAULT_HEARTBEAT   15

// default check heart beat seconds
#define NET_DEFAULT_HEARTBEAT_CHECK     7

// error code
#define NET_ERR_CONN_LIMIT      100001
#define NET_ERR_TIMEOUT         100002
#define NET_ERR_INVALID_SIZE    100003