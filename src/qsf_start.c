// Copyright (C) 2014-2015 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#include "qsf.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <zmq.h>
#include "qsf_env.h"
#include "qsf_service.h"

#define QSF_ROUTER_ADDRESS  ("inproc://qsf.router")

typedef struct qsf_context_s
{
    uv_mutex_t mutex;
    void* context;
    void* router;
}qsf_context_t;

static qsf_context_t  qsf_context;


#define CHECK_ZMQ_MSG(r)    do {                                \
                                if ((r) < 0) {                  \
                                (r) = zmq_errno();              \
                                if ((r) != EAGAIN) return (r);} \
                            }while(0)

#define CHECK_ZMQ_RET(r)    do {if ((r) != 0) return zmq_errno();} while(0)

void* qsf_create_dealer(const char* identity)
{
    void* dealer = zmq_socket(qsf_context.context, ZMQ_DEALER);
    if (dealer == NULL)
    {
        fprintf(stderr, "create dealer failed.\n");
        return NULL;
    }
    int r = zmq_setsockopt(dealer, ZMQ_IDENTITY, identity, strlen(identity));
    if (r != 0)
    {
        fprintf(stderr, "%s\n", zmq_strerror(zmq_errno()));
        return NULL;
    }
    int linger = 0;
    r = zmq_setsockopt(dealer, ZMQ_LINGER, &linger, sizeof(linger));
    if (r != 0)
    {
        fprintf(stderr, "%s\n", zmq_strerror(zmq_errno()));
        return NULL;
    }
    int max_recv_timeout = (int)qsf_getenv_int("max_recv_timeout");
    r = zmq_setsockopt(dealer, ZMQ_RCVTIMEO, &max_recv_timeout, sizeof(max_recv_timeout));
    if (r != 0)
    {
        fprintf(stderr, "%s\n", zmq_strerror(zmq_errno()));
        return NULL;
    }
    int64_t max_msg_size = qsf_getenv_int("max_ipc_msg_size");
    r = zmq_setsockopt(dealer, ZMQ_MAXMSGSIZE, &max_msg_size, sizeof(max_msg_size));
    if (r != 0)
    {
        fprintf(stderr, "%s\n", zmq_strerror(zmq_errno()));
        return NULL;
    }
    r = zmq_connect(dealer, QSF_ROUTER_ADDRESS);
    if (r != 0)
    {
        fprintf(stderr, "%s\n", zmq_strerror(zmq_errno()));
        return NULL;
    }

    return dealer;
}

static int dispatch_message(void)
{
    zmq_msg_t from;
    zmq_msg_t to;
    zmq_msg_t msg;

    void* router = qsf_context.router;
    int r = 0;

    r = zmq_msg_init(&from);
    assert(r == 0);
    r = zmq_msg_init(&to);
    assert(r == 0);
    r = zmq_msg_init(&msg);
    assert(r == 0);

    r = zmq_msg_recv(&from, router, 0);
    CHECK_ZMQ_MSG(r);
    r = zmq_msg_recv(&to, router, 0);
    CHECK_ZMQ_MSG(r);
    r = zmq_msg_recv(&msg, router, 0);
    CHECK_ZMQ_MSG(r);

    r = zmq_msg_send(&to, router, ZMQ_SNDMORE);
    CHECK_ZMQ_MSG(r);
    r = zmq_msg_send(&from, router, ZMQ_SNDMORE);
    CHECK_ZMQ_MSG(r);
    r = zmq_msg_send(&msg, router, ZMQ_SNDMORE);
    CHECK_ZMQ_MSG(r);

    r = zmq_msg_close(&msg);
    assert(r == 0);
    r = zmq_msg_close(&from);
    assert(r == 0);
    r = zmq_msg_close(&to);
    assert(r == 0);

    return 0;
}


static int qsf_init(void)
{
    void* ctx = zmq_ctx_new();
    if (ctx == NULL)
    {
        return zmq_errno();
    }

    void* router = zmq_socket(ctx, ZMQ_ROUTER);
    if (router == NULL)
    {
        return zmq_errno();
    }
    
    int linger = 0;
    int r = zmq_setsockopt(router, ZMQ_LINGER, &linger, sizeof(linger));
    CHECK_ZMQ_RET(r);

    int mandatory = 1;
    r = zmq_setsockopt(router, ZMQ_ROUTER_MANDATORY, &mandatory, sizeof(mandatory));
    CHECK_ZMQ_RET(r);

    int64_t max_msg_size = qsf_getenv_int("max_ipc_msg_size");
    r = zmq_setsockopt(router, ZMQ_MAXMSGSIZE, &max_msg_size, sizeof(max_msg_size));
    CHECK_ZMQ_RET(r);

    r = zmq_bind(router, QSF_ROUTER_ADDRESS);
    CHECK_ZMQ_RET(r);

    qsf_context.context = ctx;
    qsf_context.router = router;
    return 0;
}

static void qsf_exit(void)
{
    qsf_env_exit();
    qsf_service_exit();
    zmq_ctx_term(qsf_context.context);
}

void* qsf_zmq_context(void)
{
    return qsf_context.context;
}

int qsf_start(const char* file)
{
    int major, minor, patch;
    zmq_version(&major, &minor, &patch);
    if (major != ZMQ_VERSION_MAJOR)
    {
        fprintf(stderr, "invalid zmq version %d/%d", major, ZMQ_VERSION_MAJOR);
        return 1;
    }

    int r = 0;
    r = qsf_env_init(file);
    if (r != 0)
        return r;

    r = qsf_init();
    if (r != 0)
    {
        fprintf(stderr, "zmq error: %s\n", zmq_strerror(r));
        return r;
    }

    r = qsf_service_init();
    if (r != 0) 
        return r;

    const char* name = qsf_getenv("start_name");
    const char* path = qsf_getenv("start_file");
    r = qsf_create_service(name, path, "sys");
    if (r != 0) return r;

    while (1)
    {
        int r = dispatch_message();
        if (r != 0)
        {
            fprintf(stderr, "%s\n", zmq_strerror(r));
            break;
        }
    }
    qsf_exit();
    return 0;
}
