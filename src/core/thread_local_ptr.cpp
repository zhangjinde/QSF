// Copyright (C) 2014 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#include "thread_local_ptr.h"
#include <cassert>
#include "logging.h"

namespace detail {

#ifdef _WIN32

#include <windows.h>

tls_key_t tls_key_create()
{
    tls_key_t key = TlsAlloc();
    if (key == TLS_OUT_OF_INDEXES)
        return 0;
    return key;
}

void tls_key_free(tls_key_t key)
{
    CHECK(TlsFree(key) != 0) << GetLastError();
}

void tls_key_set(tls_key_t key, void* value)
{
    CHECK(TlsSetValue(key, value) != 0) << GetLastError();
}

void* tls_key_get(tls_key_t key)
{
    void* value = TlsGetValue(key);
    if (value == nullptr)
    {
        CHECK(GetLastError() == ERROR_SUCCESS);
    }
    return value;
}


#elif defined(__linux__) || defined(__GNUC__)

#include <pthread.h>

tls_key_t tls_key_create()
{
    uint32_t key = 0;
    CHECK(pthread_key_create((pthread_key_t*)&key, nullptr) == 0);
    return key;
}

void tls_key_free(tls_key_t key)
{
    CHECK(pthread_key_delete(key) == 0);
}

void tls_key_set(tls_key_t key, void* ptr)
{
    CHECK(pthread_setspecific(id, ptr) == 0);
}

void* tls_key_get(tls_key_t key)
{
    return pthread_getspecific(key);
}

#else

#error thread local storage not supported in this platform!

#endif

} // namespace detail
