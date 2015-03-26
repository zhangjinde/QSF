#include <gtest/gtest.h>
#include "Benchmark.h"
#include "core/Logging.h"
#include "core/CpuId.h"
#include "core/Checksum.h"

namespace detail {

uint32_t crc32c_hw(const void* data, size_t nbytes, uint32_t init_crc = 0L);
uint32_t crc32c_sw(const void* data, size_t nbytes, uint32_t init_crc = 0L);

} // namespace detail


TEST(CRC32C, StandardResults) 
{
    // From rfc3720 section B.4.
    char buf[32];

    memset(buf, 0, sizeof(buf));
    EXPECT_EQ(0x8a9136aa, crc32c(buf, sizeof(buf), 0));

    memset(buf, 0xff, sizeof(buf));
    EXPECT_EQ(0x62a8ab43, crc32c(buf, sizeof(buf), 0));

    for (int i = 0; i < 32; i++) 
    {
        buf[i] = i;
    }
    EXPECT_EQ(0x46dd794e, crc32c(buf, sizeof(buf), 0));

    for (int i = 0; i < 32; i++) 
    {
        buf[i] = 31 - i;
    }
    EXPECT_EQ(0x113fdb5c, crc32c(buf, sizeof(buf), 0));

    unsigned char data[48] = 
    {
        0x01, 0xc0, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x14, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x04, 0x00,
        0x00, 0x00, 0x00, 0x14,
        0x00, 0x00, 0x00, 0x18,
        0x28, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x02, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
    };
    EXPECT_EQ(0xd9963a56, crc32c(reinterpret_cast<char*>(data), sizeof(data), 0));
}

TEST(CRC32C, Values) 
{
    EXPECT_NE(crc32c("a", 1, 0), crc32c("foo", 3, 0));
    uint32_t checksum = crc32c("world", 5, crc32c("hello ", 6, 0));
    EXPECT_EQ(crc32c("hello world", 11, 0), checksum);
}
