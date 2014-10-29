// Copyright (C) 2014 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#pragma once

#include <cstdint>

namespace detail {

typedef uint32_t tls_key_t;

// allocates a thread local storage (TLS) index
tls_key_t tls_key_create();

// releases a thread local storage (TLS) index
void tls_key_free(tls_key_t key);

// stores a value in the calling thread's thread local storage (TLS) 
// slot for the specified TLS index
void tls_key_set(tls_key_t key, void* value);

// retrieves the value in the calling thread's thread local storage (TLS) 
// slot for the specified TLS index
void* tls_key_get(tls_key_t key);

} // namespace detail


template <typename T>
class ThreadLocalPtr
{
public:
    ThreadLocalPtr() 
        : key_(detail::tls_key_create())
    {
    }

    explicit ThreadLocalPtr(T* value)
        : key_(detail::tls_key_create())
    {
        detail::tls_key_set(key_, value);
    }

    ThreadLocalPtr(ThreadLocalPtr&& other)
        : key_(other.key_)
    {
        other.key_ = 0;
    }

    ~ThreadLocalPtr()
    {
        destroy();
    }

    // non-copyable
    ThreadLocalPtr(const ThreadLocalPtr&) = delete;
    ThreadLocalPtr& operator=(const ThreadLocalPtr&) = delete;

    T* get() const
    {
        return static_cast<T*>(detail::tls_key_get(key_));
    }

    T* operator->() const
    {
        return get();
    }

    T& operator*() const
    {
        return *get();
    }

    T* release()
    {
        T* ptr = get();
        if (ptr)
        {
            detail::tls_key_set(key_, nullptr);
        }
        return ptr;
    }

    void reset(T* ptr = nullptr)
    {
        T* old = release();
        delete old;
        if (ptr)
        {            
            detail::tls_key_set(key_, ptr);
        }
    }

    explicit operator bool() const
    {
        return get() != nullptr;
    }

private:
    void destroy()
    {
        if (key_)
        {
            T* ptr = release();
            delete ptr;
            detail::tls_key_free(key_);
            key_ = 0;
        }
    }

private:
    detail::tls_key_t   key_;
};
