/*
 * Copyright 2014 Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <climits>
#include <limits>
#include <stdexcept>
#include <gtest/gtest.h>
#include "Benchmark.h"
#include "core/Conv.h"

using namespace std;


TEST(Conv, Integral2Integral)
{
    // Same size, different signs
    int64_t s64 = numeric_limits<uint8_t>::max();
    EXPECT_EQ(to<uint8_t>(s64), s64);

    s64 = numeric_limits<int8_t>::max();
    EXPECT_EQ(to<int8_t>(s64), s64);

    s64 = numeric_limits<int32_t>::max();
    EXPECT_THROW(to<int8_t>(s64), std::range_error);    // convert large to less
}

TEST(Conv, Floating2Floating)
{
    float f1 = 1e3;
    double d1 = to<double>(f1);
    EXPECT_EQ(f1, d1);

    double d2 = 23.0;
    auto f2 = to<float>(d2);
    EXPECT_EQ(double(f2), d2);

    double invalidFloat = std::numeric_limits<double>::max();
    EXPECT_THROW(to<float>(invalidFloat), std::range_error);
    invalidFloat = -std::numeric_limits<double>::max();
    EXPECT_THROW(to<float>(invalidFloat), std::range_error);

    try
    {
        auto shouldWork = to<float>(std::numeric_limits<double>::min());
        // The value of `shouldWork' is an implementation defined choice
        // between the following two alternatives.
        EXPECT_TRUE(shouldWork == std::numeric_limits<float>::min() ||
            shouldWork == 0.f);
    }
    catch (...)
    {
        EXPECT_TRUE(false);
    }
}


TEST(Conv, testIntegral2String)
{
    char ch = 'X';
    EXPECT_EQ(to<string>(ch), "X");
    EXPECT_EQ(to<string>(uint8_t(-0)), "0");
    EXPECT_EQ(to<string>(uint8_t(UCHAR_MAX)), "255");

    EXPECT_EQ(to<string>(int16_t(-0)), "0");
    EXPECT_EQ(to<string>(int16_t(SHRT_MIN)), "-32768");
    EXPECT_EQ(to<string>(int16_t(SHRT_MAX)), "32767");
    EXPECT_EQ(to<string>(uint16_t(0)), "0");
    EXPECT_EQ(to<string>(uint16_t(USHRT_MAX)), "65535");

    EXPECT_EQ(to<string>(int32_t(-0)), "0");
    EXPECT_EQ(to<string>(int32_t(INT_MIN)), "-2147483648");
    EXPECT_EQ(to<string>(int32_t(INT_MAX)), "2147483647");
    EXPECT_EQ(to<string>(uint32_t(0)), "0");
    EXPECT_EQ(to<string>(uint32_t(UINT_MAX)), "4294967295");

    EXPECT_EQ(to<string>(int64_t(-0)), "0");
    EXPECT_EQ(to<string>(int64_t(INT64_MIN)), "-9223372036854775808");
    EXPECT_EQ(to<string>(int64_t(INT64_MAX)), "9223372036854775807");
    EXPECT_EQ(to<string>(uint64_t(0)), "0");
    EXPECT_EQ(to<string>(uint64_t(UINT64_MAX)), "18446744073709551615");

    enum A{ x = 0, y = 420, z = 2147483647 };
    EXPECT_EQ(to<string>(x), "0");
    EXPECT_EQ(to<string>(y), "420");
    EXPECT_EQ(to<string>(z), "2147483647");
}

TEST(Conv, testFloating2String)
{
    EXPECT_EQ(to<string>(0.0f), "0.0");
    EXPECT_EQ(to<string>(0.5), "0.5");
    EXPECT_EQ(to<string>(10.25f), "10.25");
    EXPECT_EQ(to<string>(1.123e10), "11230000000.0");
}

TEST(Conv, testString2String)
{
    string s = "hello, kitty";
    EXPECT_EQ(to<string>(s), s);
    StringPiece piece(s.data(), 5);
    EXPECT_EQ(to<string>(piece), "hello");
}


TEST(Conv, testString2Integral)
{
    EXPECT_EQ(to<uint8_t>(string("255")), 255);
    EXPECT_EQ(to<int16_t>(string("-32768")), -32768);
    EXPECT_EQ(to<uint16_t>(string("65535")), 65535);
    EXPECT_EQ(to<int32_t>(string("-2147483648")), -2147483648L);
    EXPECT_THROW(to<int16_t>(string("-2147483648")), std::range_error);
    EXPECT_EQ(to<uint32_t>(string("4294967295")), 4294967295);
    EXPECT_EQ(to<int64_t>(string("-9223372036854775808")), INT64_MIN);
    EXPECT_EQ(to<int64_t>(string("9223372036854775807")), INT64_MAX);
    EXPECT_EQ(to<uint64_t>(string("18446744073709551615")), UINT64_MAX);

    // empty string
    string s = "";
    StringPiece pc(s);
    EXPECT_THROW(to<int>(pc), std::range_error);

    // corrupted string
    s = "-1";
    StringPiece pc2(s.data(), s.data() + 1); // Only  "-"
    EXPECT_THROW(to<int64_t>(&pc2), std::range_error);
}

TEST(Conv, testString2Bool)
{
    EXPECT_FALSE(to<bool>(string("0")));
    EXPECT_FALSE(to<bool>(string("  000  ")));

    EXPECT_FALSE(to<bool>(string("n")));
    EXPECT_FALSE(to<bool>(string("no")));
    EXPECT_FALSE(to<bool>(string("false")));
    EXPECT_FALSE(to<bool>(string("False")));
    EXPECT_FALSE(to<bool>(string("  fAlSe")));
    EXPECT_FALSE(to<bool>(string("F")));
    EXPECT_FALSE(to<bool>(string("off")));

    EXPECT_TRUE(to<bool>(string("1")));
    EXPECT_TRUE(to<bool>(string("  001 ")));
    EXPECT_TRUE(to<bool>(string("y")));
    EXPECT_TRUE(to<bool>(string("yes")));
    EXPECT_TRUE(to<bool>(string("\nyEs\t")));
    EXPECT_TRUE(to<bool>(string("true")));
    EXPECT_TRUE(to<bool>(string("True")));
    EXPECT_TRUE(to<bool>(string("T")));
    EXPECT_TRUE(to<bool>(string("on")));

    EXPECT_THROW(to<bool>(string("")), std::range_error);
    EXPECT_THROW(to<bool>(string("2")), std::range_error);
    EXPECT_THROW(to<bool>(string("11")), std::range_error);
    EXPECT_THROW(to<bool>(string("19")), std::range_error);
    EXPECT_THROW(to<bool>(string("o")), std::range_error);
    EXPECT_THROW(to<bool>(string("fal")), std::range_error);
    EXPECT_THROW(to<bool>(string("tru")), std::range_error);
    EXPECT_THROW(to<bool>(string("ye")), std::range_error);
    EXPECT_THROW(to<bool>(string("yes foo")), std::range_error);
    EXPECT_THROW(to<bool>(string("bar no")), std::range_error);
    EXPECT_THROW(to<bool>(string("one")), std::range_error);
    EXPECT_THROW(to<bool>(string("true_")), std::range_error);
    EXPECT_THROW(to<bool>(string("bogus_token_that_is_too_long")), std::range_error);

    // Test with strings that are not NUL terminated.
    const char buf[] = "01234";
    EXPECT_FALSE(to<bool>(StringPiece(buf, buf + 1)));  // "0"
    EXPECT_TRUE(to<bool>(StringPiece(buf + 1, buf + 2)));  // "1"
    const char buf2[] = "one two three";
    EXPECT_TRUE(to<bool>(StringPiece(buf2, buf2 + 2)));  // "on"
    const char buf3[] = "false";
    EXPECT_THROW(to<bool>(StringPiece(buf3, buf3 + 3)), std::range_error); // "fal"

    // Test the StringPiece* API
    const char buf4[] = "001foo";
    StringPiece sp4(buf4);
    EXPECT_TRUE(to<bool>(&sp4));
    EXPECT_EQ(buf4 + 3, sp4.begin());
    const char buf5[] = "0012";
    StringPiece sp5(buf5);
    EXPECT_THROW(to<bool>(&sp5), std::range_error);
    EXPECT_EQ(buf5, sp5.begin());
}

TEST(Conv, StringPieceToDouble)
{
    string s = "2134123.125 zorro";
    StringPiece pc(s);
    EXPECT_EQ(to<double>(&pc), 2134123.125);
    EXPECT_EQ(pc, " zorro");

    EXPECT_THROW(to<double>(StringPiece(s)), std::range_error);
    EXPECT_EQ(to<double>(StringPiece(s.data(), pc.data())), 2134123.125);

    // Test NaN conversion
    try
    {
        to<double>("not a number");
        EXPECT_TRUE(false);
    }
    catch (const std::exception &)
    {
    }

    EXPECT_TRUE(std::isnan(to<double>("NaN")));
    EXPECT_EQ(to<double>("inf"), numeric_limits<double>::infinity());
    EXPECT_EQ(to<double>("infinity"), numeric_limits<double>::infinity());
    EXPECT_THROW(to<double>("infinitX"), std::range_error);
    EXPECT_EQ(to<double>("-inf"), -numeric_limits<double>::infinity());
    EXPECT_EQ(to<double>("-infinity"), -numeric_limits<double>::infinity());
    EXPECT_THROW(to<double>("-infinitX"), std::range_error);

    EXPECT_THROW(to<double>(""), std::range_error); // empty string to double
}

TEST(Conv, testVariadicTo)
{
    string s;
    toAppend(&s);
    toAppend(&s, "Lorem ipsum ", 1234, string(" dolor amet "), 567.89, '!');
    EXPECT_EQ(s, "Lorem ipsum 1234 dolor amet 567.89!");

    s = to<string>("");
    EXPECT_TRUE(s.empty());

    s = to<string>("Lorem ipsum ", nullptr, 1234, " dolor amet ", 567.89, '.');
    EXPECT_EQ(s, "Lorem ipsum 1234 dolor amet 567.89.");
}

TEST(Conv, testVariadicToDelim)
{
    string s("Yukkuri shiteitte ne!!!");

    string charDelim = toDelim<string>('$', s);
    EXPECT_EQ(charDelim, s);

    string strDelim = toDelim<string>(string(">_<"), s);
    EXPECT_EQ(strDelim, s);

    s.clear();
    toAppendDelim(&s,
        ":", "Lorem ipsum ", 1234, string(" dolor amet "), 567.89, '!');
    EXPECT_EQ(s, "Lorem ipsum :1234: dolor amet :567.89:!");

    s = toDelim<string>(':', "");
    EXPECT_TRUE(s.empty());

    s = toDelim<string>(
        ":", "Lorem ipsum ", nullptr, 1234, " dolor amet ", 567.89, '.');
    EXPECT_EQ(s, "Lorem ipsum ::1234: dolor amet :567.89:.");
}

TEST(Conv, testEnum)
{
    enum A { x = 42, y = 420, z = 65 };
    auto i = to<int>(x);
    EXPECT_EQ(i, 42);
    auto j = to<char>(x);
    EXPECT_EQ(j, 42);
    try
    {
        auto i = to<char>(y);
        LOG(ERROR) << static_cast<unsigned int>(i);
        EXPECT_TRUE(false);
    }
    catch (std::exception& e)
    {
        //LOG(INFO) << e.what();
    }

    EXPECT_EQ("42", to<string>(x));
    EXPECT_EQ("420", to<string>(y));
    EXPECT_EQ("65", to<string>(z));

    auto i2 = to<A>(42);
    EXPECT_EQ(i, x);
    auto j2 = to<A>(100);
    EXPECT_EQ(j2, 100);
    try 
    {
        auto i = to<A>(5000000000L);
        EXPECT_TRUE(false);
    }
    catch (std::exception& e)
    {
        //LOG(INFO) << e.what();
    }

    enum E : uint32_t { m = 3000000000U };
    auto u = to<uint32_t>(m);
    EXPECT_EQ(u, 3000000000U);
    auto s = to<string>(m);
    EXPECT_EQ("3000000000", s);
    auto e = to<E>(3000000000U);
    EXPECT_EQ(e, m);
    try
    {
        auto i = to<int32_t>(m);
        LOG(ERROR) << to<uint32_t>(m);
        EXPECT_TRUE(false);
    }
    catch (std::exception& e)
    {
    }
}

TEST(Conv, NewUint64ToString)
{
    char buf[21];

#define THE_GREAT_EXPECTATIONS(n, len)                      \
    do {                                                    \
        EXPECT_EQ((len), uint64ToBufferUnsafe((n), buf));   \
        buf[(len)] = 0;                                     \
        auto s = string(#n);                                \
        s = s.substr(0, s.size() - 2);                      \
        EXPECT_EQ(s, buf);                                  \
        } while (0)

    THE_GREAT_EXPECTATIONS(0UL, 1);
    THE_GREAT_EXPECTATIONS(1UL, 1);
    THE_GREAT_EXPECTATIONS(12UL, 2);
    THE_GREAT_EXPECTATIONS(123UL, 3);
    THE_GREAT_EXPECTATIONS(1234UL, 4);
    THE_GREAT_EXPECTATIONS(12345UL, 5);
    THE_GREAT_EXPECTATIONS(123456UL, 6);
    THE_GREAT_EXPECTATIONS(1234567UL, 7);
    THE_GREAT_EXPECTATIONS(12345678UL, 8);
    THE_GREAT_EXPECTATIONS(123456789UL, 9);
    THE_GREAT_EXPECTATIONS(1234567890UL, 10);
    THE_GREAT_EXPECTATIONS(12345678901UL, 11);
    THE_GREAT_EXPECTATIONS(123456789012UL, 12);
    THE_GREAT_EXPECTATIONS(1234567890123UL, 13);
    THE_GREAT_EXPECTATIONS(12345678901234UL, 14);
    THE_GREAT_EXPECTATIONS(123456789012345UL, 15);
    THE_GREAT_EXPECTATIONS(1234567890123456UL, 16);
    THE_GREAT_EXPECTATIONS(12345678901234567UL, 17);
    THE_GREAT_EXPECTATIONS(123456789012345678UL, 18);
    THE_GREAT_EXPECTATIONS(1234567890123456789UL, 19);
    THE_GREAT_EXPECTATIONS(18446744073709551614UL, 20);
    THE_GREAT_EXPECTATIONS(18446744073709551615UL, 20);

#undef THE_GREAT_EXPECTATIONS
}

////////////////////////////////////////////////////////////////////////////////
// Benchmarks for ASCII to int conversion
////////////////////////////////////////////////////////////////////////////////
// @author: Rajat Goel (rajat)

static int64_t handwrittenAtoi(const char* start, const char* end) 
{
    bool positive = true;
    int64_t retVal = 0;

    if (start == end)
    {
        throw std::runtime_error("empty string");
    }

    while (start < end && isspace(*start))
    {
        ++start;
    }
    switch (*start)
    {
    case '-':
        positive = false;
    case '+':
        ++start;
    default:;
    }
    while (start < end && *start >= '0' && *start <= '9')
    {
        auto const newRetVal = retVal * 10 + (*start++ - '0');
        if (newRetVal < retVal)
        {
            throw std::runtime_error("overflow");
        }
        retVal = newRetVal;
    }
    if (start != end)
    {
        throw std::runtime_error("extra chars at the end");
    }
    return positive ? retVal : -retVal;
}

static StringPiece pc1 = "1234567890123456789";

BENCHMARK(clibAtoi, n)
{
    for (int digits = 1; digits < 20; ++digits)
    {
        auto p = pc1.subpiece(pc1.size() - digits, digits);
        assert(*p.end() == 0);
        //static_assert(sizeof(long) == 8, "64-bit long assumed");
        for (unsigned i = 0; i < n; i++)
        {
            doNotOptimizeAway(atol(p.begin()));
        }
    }
}

BENCHMARK_RELATIVE(clibStrtoul, n)
{
    for (int digits = 1; digits < 20; ++digits)
    {
        auto p = pc1.subpiece(pc1.size() - digits, digits);
        assert(*p.end() == 0);
        char * endptr;
        for (unsigned i = 0; i < n; i++) {
            doNotOptimizeAway(strtoul(p.begin(), &endptr, 10));
        }
    }
}

BENCHMARK_RELATIVE(handwrittenAtoi, n)
{
    for (int digits = 1; digits < 20; ++digits)
    {
        auto p = pc1.subpiece(pc1.size() - digits, digits);
        for (unsigned i = 0; i < n; i++)
        {
            doNotOptimizeAway(handwrittenAtoi(p.begin(), p.end()));
        }
    }
}

BENCHMARK_RELATIVE(follyAtoi, n)
{
    for (int digits = 1; digits < 20; ++digits)
    {
        auto p = pc1.subpiece(pc1.size() - digits, digits);
        for (unsigned i = 0; i < n; i++)
        {
            doNotOptimizeAway(to<int64_t>(p.begin(), p.end()));
        }
    }
}

// Benchmarks for unsigned to string conversion, raw
unsigned u64ToAsciiTable(uint64_t value, char* dst) 
{
    static const char digits[201] =
        "00010203040506070809"
        "10111213141516171819"
        "20212223242526272829"
        "30313233343536373839"
        "40414243444546474849"
        "50515253545556575859"
        "60616263646566676869"
        "70717273747576777879"
        "80818283848586878889"
        "90919293949596979899";

    uint32_t const length = digits10(value);
    uint32_t next = length - 1;
    while (value >= 100)
    {
        auto const i = (value % 100) * 2;
        value /= 100;
        dst[next] = digits[i + 1];
        dst[next - 1] = digits[i];
        next -= 2;
    }
    // Handle last 1-2 digits
    if (value < 10)
    {
        dst[next] = '0' + uint32_t(value);
    }
    else
    {
        auto i = uint32_t(value) * 2;
        dst[next] = digits[i + 1];
        dst[next - 1] = digits[i];
    }
    return length;
}

unsigned u64ToAsciiClassic(uint64_t value, char* dst)
{
    // Write backwards.
    char* next = (char*)dst;
    char* start = next;
    do
    {
        *next++ = '0' + (value % 10);
        value /= 10;
    } while (value != 0);
    unsigned length = next - start;

    // Reverse in-place.
    next--;
    while (next > start)
    {
        char swap = *next;
        *next = *start;
        *start = swap;
        next--;
        start++;
    }
    return length;
}

BENCHMARK_DRAW_LINE();

const static uint64_t valueTable[] =
{
    (1),
    (12),
    (123),
    (1234),
    (12345),
    (123456),
    (1234567),
    (12345678),
    (123456789),
    (1234567890),
    (12345678901),
    (123456789012),
    (1234567890123),
    (12345678901234),
    (123456789012345),
    (1234567890123456),
    (12345678901234567),
    (123456789012345678),
    (1234567890123456789),
    (12345678901234567890U),
};

BENCHMARK(u64ToAsciiTableBM, n)
{
    for (auto value : valueTable)
    {
        // This is too fast, need to do 10 times per iteration
        char buf[20];
        for (unsigned i = 0; i < n; i++)
        {
            doNotOptimizeAway(u64ToAsciiTable(value + n, buf));
        }
    }
}

BENCHMARK_RELATIVE(u64ToAsciiClassicBM, n)
{
    for (auto value : valueTable)
    {
        // This is too fast, need to do 10 times per iteration
        char buf[20];
        for (unsigned i = 0; i < n; i++)
        {
            doNotOptimizeAway(u64ToAsciiClassic(value + n, buf));
        }
    }
}

BENCHMARK_RELATIVE(u64ToAsciiFollyBM, n)
{
    for (auto value : valueTable)
    {
        // This is too fast, need to do 10 times per iteration
        char buf[20];
        for (unsigned i = 0; i < n; i++)
        {
            doNotOptimizeAway(uint64ToBufferUnsafe(value + n, buf));
        }
    }
}

BENCHMARK_DRAW_LINE();

// Benchmark uitoa with string append

void u2aAppendClassicBM(unsigned int n, uint64_t value) 
{
    string s;
    for (unsigned i = 0; i < n; i++) 
    {
        // auto buf = &s.back() + 1;
        char buffer[20];
        s.append(buffer, u64ToAsciiClassic(value, buffer));
        doNotOptimizeAway(s.size());
    }
}

void u2aAppendFollyBM(unsigned int n, uint64_t value) 
{
    string s;
    for (unsigned i = 0; i < n; i++)
    {
        // auto buf = &s.back() + 1;
        char buffer[20];
        s.append(buffer, uint64ToBufferUnsafe(value, buffer));
        doNotOptimizeAway(s.size());
    }
}

const static uint64_t bigInt = 11424545345345;
const static size_t smallInt = 104;
const static char someString[] = "this is some nice string";
const static char otherString[] = "this is a long string, so it's not so nice";
const static char reallyShort[] = "meh";
const static std::string stdString = "std::strings are very nice";
const static float fValue = 1.2355f;
const static double dValue = 345345345.435;

BENCHMARK(VariadicTestNoFloat, n)
{
    for (unsigned i = 0; i < n; ++i)
    {
        auto val1 = to<string>(bigInt, someString, stdString, otherString);
        auto val3 = to<string>(reallyShort, smallInt);
        auto val2 = to<string>(bigInt, stdString);
        auto val4 = to<string>(bigInt, stdString, dValue, otherString);
        auto val5 = to<string>(bigInt, someString, reallyShort);
        doNotOptimizeAway(val1);
        doNotOptimizeAway(val3);
        doNotOptimizeAway(val2);
        doNotOptimizeAway(val4);
        doNotOptimizeAway(val5);
    }
}

BENCHMARK(VariadicTestFloat, n)
{
    for (unsigned i = 0; i < n; ++i)
    {
        auto val1 = to<string>(stdString, ',', fValue, dValue);
        auto val2 = to<string>(stdString, ',', dValue);
        doNotOptimizeAway(val1);
        doNotOptimizeAway(val2);
    }
}

BENCHMARK_DRAW_LINE();

static const int sizeTable[] = 
{
    32, 1024, 32768,
};

BENCHMARK(StringIdenticalToBM,  n)
{
    for (auto len : sizeTable)
    {
        string s;
        BENCHMARK_SUSPEND{ s.append(len, '0'); }
        for (unsigned i = 0; i < n; i++)
        {
            string result = to<string>(s);
            doNotOptimizeAway(result.size());
        }
    }
}

BENCHMARK(StringVariadicToBM, n)
{
    for (auto len : sizeTable)
    {
        string s;
        BENCHMARK_SUSPEND{ s.append(len, '0'); }
        for (unsigned i = 0; i < n; i++)
        {
            string result = to<string>(s, nullptr);
            doNotOptimizeAway(result.size());
        }
    }
}

BENCHMARK_DRAW_LINE();
