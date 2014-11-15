#include <gtest/gtest.h>
#include "core/benchmark.h"
#include "core/logging.h"
#include "core/cpuid.h"
#include "core/checksum.h"

namespace detail {

uint32_t crc32c_hw(const void* data, size_t nbytes, uint32_t init_crc = 0L);
uint32_t crc32c_sw(const void* data, size_t nbytes, uint32_t init_crc = 0L);

} // namespace detail


TEST(CRC32C, StandardResults) 
{
    // From rfc3720 section B.4.
    char buf[32];

    memset(buf, 0, sizeof(buf));
    EXPECT_EQ(0x8a9136aa, crc32c(buf, sizeof(buf)));

    memset(buf, 0xff, sizeof(buf));
    EXPECT_EQ(0x62a8ab43, crc32c(buf, sizeof(buf)));

    for (int i = 0; i < 32; i++) 
    {
        buf[i] = i;
    }
    EXPECT_EQ(0x46dd794e, crc32c(buf, sizeof(buf)));

    for (int i = 0; i < 32; i++) 
    {
        buf[i] = 31 - i;
    }
    EXPECT_EQ(0x113fdb5c, crc32c(buf, sizeof(buf)));

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
    EXPECT_EQ(0xd9963a56, crc32c(reinterpret_cast<char*>(data), sizeof(data)));
}

TEST(CRC32C, Values) 
{
    EXPECT_NE(crc32c("a", 1), crc32c("foo", 3));
    uint32_t checksum = crc32c("world", 5, crc32c("hello ", 6));
    EXPECT_EQ(crc32c("hello world", 11), checksum);
}

namespace {

const unsigned int BUFFER_SIZE = 512 * 1024 * sizeof(uint64_t);
uint8_t buffer[BUFFER_SIZE];

BENCHMARK_INITIALIZER(generateRandomValues)
{
    // Populate a buffer with a deterministic pattern
    // on which to compute checksums
    srand((unsigned)time(NULL));
    const uint8_t* src = buffer;
    uint64_t* dst = (uint64_t*)buffer;
    const uint64_t* end = (const uint64_t*)(buffer + BUFFER_SIZE);
    *dst++ = 0;
    while (dst < end) 
    {
        *dst++ = rand();
        src += sizeof(uint64_t);
    }
}

static CpuId cpuid;


void benchmarkHardwareCRC32C(unsigned long iters, size_t blockSize) 
{
    if (cpuid.sse42())
    {
        uint32_t checksum;
        for (unsigned long i = 0; i < iters; i++) 
        {
            checksum = detail::crc32c_hw(buffer, blockSize);
            doNotOptimizeAway(checksum);
        }
    }
    else 
    {
        LOG(WARNING) << "skipping hardware-accelerated CRC-32C benchmarks" <<
            " (not supported on this CPU)";
    }
}

void benchmarkSoftwareCRC32C(unsigned long iters, size_t blockSize) 
{
    uint32_t checksum;
    for (unsigned long i = 0; i < iters; i++) 
    {
        checksum = detail::crc32c_sw(buffer, blockSize);
        doNotOptimizeAway(checksum);
    }
}

} // anonymouse namespace


// This test fits easily in the L1 cache on modern server processors,
// and thus it mainly measures the speed of the checksum computation.
BENCHMARK(crc32c_hardware_1KB_block, iters) 
{
    benchmarkHardwareCRC32C(iters, 1024);
}

BENCHMARK(crc32c_software_1KB_block, iters) 
{
    benchmarkSoftwareCRC32C(iters, 1024);
}

BENCHMARK_DRAW_LINE();

// This test is too big for the L1 cache but fits in L2
BENCHMARK(crc32c_hardware_64KB_block, iters) 
{
    benchmarkHardwareCRC32C(iters, 64 * 1024);
}

BENCHMARK(crc32c_software_64KB_block, iters) 
{
    benchmarkSoftwareCRC32C(iters, 64 * 1024);
}

BENCHMARK_DRAW_LINE();

// This test is too big for the L2 cache but fits in L3
BENCHMARK(crc32c_hardware_512KB_block, iters) 
{
    benchmarkHardwareCRC32C(iters, 512 * 1024);
}

BENCHMARK(crc32c_software_512KB_block, iters) 
{
    benchmarkSoftwareCRC32C(iters, 512 * 1024);
}
