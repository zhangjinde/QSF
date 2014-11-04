#pragma once

#include <cstdint>
#include <memory>
#include <functional>
#include <boost/noncopyable.hpp>
#include "core/range.h"


class IOBuf : boost::noncopyable
{
public:
    enum CreateOp { CREATE };
    enum TakeOwnershipOp { TAKE_OWNERSHIP };
    enum CopyBufferOp { COPY_BUFFER };

    typedef void(*FreeFunction)(void* buf);

    static std::unique_ptr<IOBuf> create(size_t capacity);
    static std::unique_ptr<IOBuf> copyBuffer(const void* buf, size_t size);
    static std::unique_ptr<IOBuf> copyBuffer(ByteRange data);
    static std::unique_ptr<IOBuf> takeOwnership(void* data, size_t size, FreeFunction fn);

    IOBuf(CreateOp, size_t capacity);
    IOBuf(CopyBufferOp, const void* data, size_t capacity);
    IOBuf(TakeOwnershipOp, void* data, size_t capacity, FreeFunction fn);

    ~IOBuf();

    IOBuf(IOBuf&& other) noexcept;
    IOBuf& operator=(IOBuf&& other) noexcept;

    const uint8_t* buffer() const { return data_; }
    uint8_t* buffer() { return data_; }

    size_t length() const { return length_; }
    bool empty() const { return length() == 0U; }
    size_t capacity() const { return capacity_ - length(); }

    const uint8_t* data() const { return buffer() + length(); }
    uint8_t* data() { return buffer() + length(); }

    ByteRange range() { return ByteRange(buffer(), length()); }
    
    void append(size_t amount)
    {
        DCHECK(length_ + amount <= capacity_);
        length_ += amount;
    }

private:
    uint8_t*        data_ = nullptr;
    const size_t    capacity_ = 0;
    size_t          length_ = 0;
    FreeFunction    freefn_ = nullptr;
};
