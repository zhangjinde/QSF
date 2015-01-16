#include <gtest/gtest.h>
#include <string>
#include "net/IOBuf.h"

using namespace std;
using namespace net;

inline void addToBuffer(std::shared_ptr<IOBuf> buf, const void* data, size_t size)
{
    assert(data && buf->capacity() >= size);
    memcpy(buf->buffer(), data, size);
    buf->append(size);
}

TEST(IOBuf, Create)
{
    auto buf = IOBuf::create(1024);
    EXPECT_NE(buf->buffer(), nullptr);
    EXPECT_NE(buf->data(), nullptr);
    EXPECT_EQ(buf->capacity(), 1024);
    EXPECT_EQ(buf->length(), 0);
    EXPECT_TRUE(buf->empty());
    EXPECT_EQ(buf->buffer(), buf->data());

    string msg = "a quick fox jumps over the lazy dog";
    addToBuffer(buf, msg.data(), msg.length());
    EXPECT_EQ(buf->length(), msg.length());
    EXPECT_EQ(buf->capacity(), 1024-msg.length());
    StringPiece s(buf->byteRange());
    EXPECT_EQ(s, msg);
}

TEST(IOBuf, copyBuffer)
{
    string msg = "a quick fox jumps over the lazy dog";
    auto buf = IOBuf::copyBuffer(msg.data(), msg.length());
    EXPECT_NE(buf->buffer(), reinterpret_cast<const uint8_t*>(msg.data()));
    EXPECT_EQ(buf->capacity(), 0);
    EXPECT_EQ(buf->length(), msg.length());
    EXPECT_FALSE(buf->empty());
    StringPiece s(buf->byteRange());
    EXPECT_EQ(s, msg);
}

TEST(IOBuf, takeOwnership)
{
    char msg[] = "a quick fox jumps over the lazy dog";
    size_t msg_size = strlen(msg);
    auto buf = IOBuf::takeOwnership(msg, msg_size, [](void*){});
    EXPECT_EQ(buf->buffer(), reinterpret_cast<const uint8_t*>(msg));
    EXPECT_EQ(buf->capacity(), 0);
    EXPECT_EQ(buf->length(), msg_size);
    EXPECT_FALSE(buf->empty());

    StringPiece s(buf->byteRange());
    EXPECT_EQ(s, msg);

    *buf->buffer() = 'A';
    EXPECT_EQ(msg[0], 'A');
}
