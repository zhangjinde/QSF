// Copyright (C) 2014-2015 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#include "qsf.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <zmq.h>
#include "qsf_log.h"
#include "qsf_env.h"
#include "qsf_service.h"

#define QSF_ROUTER_ADDRESS  ("inproc://qsf.router")

#define qsf_zmq_assert(cond) \
    qsf_assert((cond), "zmq error: %d, %s", zmq_errno(), zmq_strerror(zmq_errno()))


typedef struct qsf_context_s
{
    void* context;
    void* router;
}qsf_context_t;

// global  qsf context object
static qsf_context_t  qsf_context;

// create a zmq dealer object connected to router
void* qsf_create_dealer(const char* identity)
{
    assert(identity);
    void* dealer = zmq_socket(qsf_context.context, ZMQ_DEALER);
    qsf_zmq_assert(dealer != NULL);

    int r = zmq_setsockopt(dealer, ZMQ_IDENTITY, identity, strlen(identity));
    qsf_zmq_assert(r == 0);

    int linger = 0;
    r = zmq_setsockopt(dealer, ZMQ_LINGER, &linger, sizeof(linger));
    qsf_zmq_assert(r == 0);

    int max_recv_timeout = (int)qsf_getenv_int("max_recv_timeout");
    r = zmq_setsockopt(dealer, ZMQ_RCVTIMEO, &max_recv_timeout, sizeof(max_recv_timeout));
    qsf_zmq_assert(r == 0);

    int64_t max_msg_size = qsf_getenv_int("max_ipc_msg_size");
    r = zmq_setsockopt(dealer, ZMQ_MAXMSGSIZE, &max_msg_size, sizeof(max_msg_size));
    qsf_zmq_assert(r == 0);

    r = zmq_connect(dealer, QSF_ROUTER_ADDRESS);
    qsf_zmq_assert(r == 0);

    return dealer;
}

// dispatch dealer message to peer
static int dispatch_message(void)
{
    zmq_msg_t from;
    zmq_msg_t to;
    zmq_msg_t msg;

    void* router = qsf_context.router;
    assert(router != NULL);

    qsf_zmq_assert(zmq_msg_init(&from) == 0);
    qsf_zmq_assert(zmq_msg_init(&to) == 0);
    qsf_zmq_assert(zmq_msg_init(&msg) == 0);

    int bytes = zmq_msg_recv(&from, router, 0);
    if (bytes < 0) 
        goto cleanup;
    bytes = zmq_msg_recv(&to, router, 0);
    if (bytes < 0) 
        goto cleanup;
    bytes = zmq_msg_recv(&msg, router, 0);
    if (bytes < 0) 
        goto cleanup;

    bytes = zmq_msg_send(&to, router, ZMQ_SNDMORE);
    if (bytes < 0) 
        goto cleanup;
    bytes = zmq_msg_send(&from, router, ZMQ_SNDMORE);
    if (bytes < 0) 
        goto cleanup;
    bytes = zmq_msg_send(&msg, router, 0);
    if (bytes < 0) 
        goto cleanup;

cleanup:
    qsf_zmq_assert(zmq_msg_close(&msg) == 0);
    qsf_zmq_assert(zmq_msg_close(&to) == 0);
    qsf_zmq_assert(zmq_msg_close(&from) == 0);

    return bytes;
}

// initialize qsf framework
static void qsf_init(void)
{
    void* ctx = zmq_ctx_new();
    qsf_zmq_assert(ctx != NULL);

    void* router = zmq_socket(ctx, ZMQ_ROUTER);
    qsf_zmq_assert(router != NULL);
    
    int linger = 0;
    int r = zmq_setsockopt(router, ZMQ_LINGER, &linger, sizeof(linger));
    qsf_zmq_assert(r == 0);

    int mandatory = 1;
    r = zmq_setsockopt(router, ZMQ_ROUTER_MANDATORY, &mandatory, sizeof(mandatory));
    qsf_zmq_assert(r == 0);

    int64_t max_msg_size = qsf_getenv_int("max_ipc_msg_size");
    r = zmq_setsockopt(router, ZMQ_MAXMSGSIZE, &max_msg_size, sizeof(max_msg_size));
    qsf_zmq_assert(r == 0);

    r = zmq_bind(router, QSF_ROUTER_ADDRESS);
    qsf_zmq_assert(r == 0);

    qsf_context.context = ctx;
    qsf_context.router = router;
}

// cleanup qsf framework
static void qsf_exit(void)
{
    qsf_env_exit();
    qsf_service_exit();
    zmq_ctx_term(qsf_context.context);
}

// global zmq context object
void* qsf_zmq_context(void)
{
    assert(qsf_context.context);
    return qsf_context.context;
}

// start qsf framework with a config file
int qsf_start(const char* file)
{
    int major, minor, patch;
    zmq_version(&major, &minor, &patch);
    qsf_assert(major == ZMQ_VERSION_MAJOR, "invalid zmq version %d/%d.", major, ZMQ_VERSION_MAJOR);

    int r = qsf_env_init(file);
    qsf_assert(r == 0, "qsf_env_init() failed.");

    qsf_init();
    
    r = qsf_service_init();
    qsf_assert(r == 0, "qsf_service_init() failed.");

    const char* name = qsf_getenv("start_name");
    const char* path = qsf_getenv("start_file");
    r = qsf_create_service(name, path, "sys");
    qsf_assert(r == 0, "create service 'sys' failed, %d.", r);

    while (1)
    {
        int r = dispatch_message();
        if (r < 0)
        {
            qsf_log("zmq message error: %d, %s", zmq_errno(), zmq_strerror(zmq_errno()));
            break;
        }
    }
    qsf_exit();
    return 0;
}
