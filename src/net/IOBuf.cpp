// Copyright (C) 2014 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#include "IOBuf.h"
#include <malloc.h>

namespace net {

std::shared_ptr<IOBuf> IOBuf::create(size_t capacity)
{
    return std::make_shared<IOBuf>(IOBuf::CREATE, capacity);
}

std::shared_ptr<IOBuf> IOBuf::copyBuffer(const void* buf, size_t size)
{
    return std::make_shared<IOBuf>(IOBuf::COPY_BUFFER, buf, size);
}

std::shared_ptr<IOBuf> IOBuf::copyBuffer(ByteRange r)
{
    return std::make_shared<IOBuf>(IOBuf::COPY_BUFFER, r.data(), r.size());
}

std::shared_ptr<IOBuf> IOBuf::takeOwnership(void* data, size_t size, FreeFunction fn)
{
    return std::make_shared<IOBuf>(IOBuf::TAKE_OWNERSHIP, data, size, fn);
}

IOBuf::IOBuf(CreateOp, size_t capacity)
    : capacity_(capacity)
{
    data_ = reinterpret_cast<uint8_t*>(malloc(capacity));
    if (data_ == nullptr)
    {
        throw std::bad_alloc();
    }
}

IOBuf::IOBuf(CopyBufferOp, const void* data, size_t size)
    : capacity_(size), length_(size)
{
    data_ = reinterpret_cast<uint8_t*>(malloc(size));
    if (data_ == nullptr)
    {
        throw std::bad_alloc();
    }
    memcpy(data_, data, size);
}

IOBuf::IOBuf(TakeOwnershipOp, void* data, size_t size, FreeFunction fn)
    : capacity_(size), length_(size), freefn_(fn)
{
    data_ = reinterpret_cast<uint8_t*>(data);
}

IOBuf::~IOBuf()
{
    if (freefn_)
    {
        freefn_(data_);
    }
    else
    {
        free(data_);
    }
    data_ = nullptr;
}

} // namespace net
