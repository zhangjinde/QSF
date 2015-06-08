// Copyright (C) 2014-2015 chenqiang@chaoyuehudong.com. All rights reserved.
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
 
// max alive connections per server
#define NET_DEFAULT_MAX_CONN    5000

// default heart beat seconds
#define NET_DEFAULT_HEARTBEAT   30

// default check heart beat seconds
#define NET_DEFAULT_HEARTBEAT_CHECK     10

// error code
#define NET_ERR_CONN_LIMIT      100001
#define NET_ERR_TIMEOUT         100002
#define NET_ERR_INVALID_SIZE    100003
