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

// @author Kristina Holst (kholst@fb.com)
// @author Andrei Alexandrescu (andrei.alexandrescu@fb.com)

#include <array>
#include <cstdlib>
#include <iterator>
#include <limits>
#include <string>
#include <vector>
#include <random>
#include <gtest/gtest.h>
#include "Benchmark.h"
#include "core/Range.h"


using namespace std;

using namespace std;


TEST(StringPiece, All)
{
    const char* foo = "foo";
    const char* foo2 = "foo";
    string fooStr(foo);
    string foo2Str(foo2);

    // comparison rather than lexical

    // the string object creates copies though, so the c_str of these should be
    // distinct
    EXPECT_NE(fooStr.c_str(), foo2Str.c_str());

    // test the basic StringPiece functionality
    StringPiece s(foo);
    EXPECT_EQ(s.size(), 3);

    EXPECT_EQ(s.start(), foo);              // ptr comparison
    EXPECT_NE(s.start(), fooStr.c_str());   // ptr comparison
    EXPECT_NE(s.start(), foo2Str.c_str());  // ptr comparison

    EXPECT_EQ(s.toString(), foo);              // lexical comparison
    EXPECT_EQ(s.toString(), fooStr.c_str());   // lexical comparison
    EXPECT_EQ(s.toString(), foo2Str.c_str());  // lexical comparison

    EXPECT_EQ(s, foo);                      // lexical comparison
    EXPECT_EQ(s, fooStr);                   // lexical comparison
    EXPECT_EQ(s, foo2Str);                  // lexical comparison
    EXPECT_EQ(foo, s);

    // check using StringPiece to reference substrings
    const char* foobarbaz = "foobarbaz";

    // the full "foobarbaz"
    s.reset(foobarbaz, strlen(foobarbaz));
    EXPECT_EQ(s.size(), 9);
    EXPECT_EQ(s.start(), foobarbaz);
    EXPECT_EQ(s, "foobarbaz");

    // only the 'foo'
    s.assign(foobarbaz, foobarbaz + 3);
    EXPECT_EQ(s.size(), 3);
    EXPECT_EQ(s.start(), foobarbaz);
    EXPECT_EQ(s, "foo");

    // just "barbaz"
    s.reset(foobarbaz + 3, strlen(foobarbaz + 3));
    EXPECT_EQ(s.size(), 6);
    EXPECT_EQ(s.start(), foobarbaz + 3);
    EXPECT_EQ(s, "barbaz");

    // just "bar"
    s.reset(foobarbaz + 3, 3);
    EXPECT_EQ(s.size(), 3);
    EXPECT_EQ(s, "bar");

    // clear
    s.clear();
    EXPECT_EQ(s.toString(), "");

    // test an empty StringPiece
    StringPiece s2;
    EXPECT_EQ(s2.size(), 0);

    // Test comparison operators
    foo = "";
    EXPECT_LE(s, foo);
    EXPECT_LE(foo, s);
    EXPECT_GE(s, foo);
    EXPECT_GE(foo, s);
    EXPECT_EQ(s, foo);
    EXPECT_EQ(foo, s);

    foo = "abc";
    EXPECT_LE(s, foo);
    EXPECT_LT(s, foo);
    EXPECT_GE(foo, s);
    EXPECT_GT(foo, s);
    EXPECT_NE(s, foo);

    EXPECT_LE(s, s);
    EXPECT_LE(s, s);
    EXPECT_GE(s, s);
    EXPECT_GE(s, s);
    EXPECT_EQ(s, s);
    EXPECT_EQ(s, s);

    s = "abc";
    s2 = "abc";
    EXPECT_LE(s, s2);
    EXPECT_LE(s2, s);
    EXPECT_GE(s, s2);
    EXPECT_GE(s2, s);
    EXPECT_EQ(s, s2);
    EXPECT_EQ(s2, s);
}

template <class T>
void expectLT(const T& a, const T& b)
{
    EXPECT_TRUE(a < b);
    EXPECT_TRUE(a <= b);
    EXPECT_FALSE(a == b);
    EXPECT_FALSE(a >= b);
    EXPECT_FALSE(a > b);

    EXPECT_FALSE(b < a);
    EXPECT_FALSE(b <= a);
    EXPECT_TRUE(b >= a);
    EXPECT_TRUE(b > a);
}

template <class T>
void expectEQ(const T& a, const T& b)
{
    EXPECT_FALSE(a < b);
    EXPECT_TRUE(a <= b);
    EXPECT_TRUE(a == b);
    EXPECT_TRUE(a >= b);
    EXPECT_FALSE(a > b);
}

TEST(StringPiece, EightBitComparisons)
{
    char values[] = { '\x00', '\x20', '\x40', '\x7f', '\x80', '\xc0', '\xff' };
    const size_t count = sizeof(values) / sizeof(values[0]);
    for (size_t i = 0; i < count; ++i) {
        std::string a(1, values[i]);
        // Defeat copy-on-write
        std::string aCopy(a.data(), a.size());
        expectEQ(a, aCopy);
        expectEQ(StringPiece(a), StringPiece(aCopy));

        for (size_t j = i + 1; j < count; ++j) {
            std::string b(1, values[j]);
            expectLT(a, b);
            expectLT(StringPiece(a), StringPiece(b));
        }
    }
}

TEST(StringPiece, ToByteRange)
{
    StringPiece a("hello");
    ByteRange b(a);
    EXPECT_EQ(static_cast<const void*>(a.begin()),
        static_cast<const void*>(b.begin()));
    EXPECT_EQ(static_cast<const void*>(a.end()),
        static_cast<const void*>(b.end()));

    // and convert back again
    StringPiece c(b);
    EXPECT_EQ(a.begin(), c.begin());
    EXPECT_EQ(a.end(), c.end());
}

TEST(StringPiece, InvalidRange)
{
    StringPiece a("hello");
    EXPECT_EQ(a, a.subpiece(0, 10));
    EXPECT_EQ(StringPiece("ello"), a.subpiece(1));
    EXPECT_EQ(StringPiece("ello"), a.subpiece(1, std::string::npos));
    EXPECT_EQ(StringPiece("ell"), a.subpiece(1, 3));
    EXPECT_ANY_THROW(a.subpiece(6, 7));
    EXPECT_ANY_THROW(a.subpiece(6));

    std::string b("hello");
    EXPECT_EQ(a, StringPiece(b, 0, 10));
    EXPECT_EQ("ello", a.subpiece(1));
    EXPECT_EQ("ello", a.subpiece(1, std::string::npos));
    EXPECT_EQ("ell", a.subpiece(1, 3));
    EXPECT_ANY_THROW(a.subpiece(6, 7));
    EXPECT_ANY_THROW(a.subpiece(6));
}

TEST(StringPiece, Prefix)
{
    StringPiece a("hello");
    EXPECT_TRUE(a.startsWith(""));
    EXPECT_TRUE(a.startsWith("h"));
    EXPECT_TRUE(a.startsWith('h'));
    EXPECT_TRUE(a.startsWith("hello"));
    EXPECT_FALSE(a.startsWith("hellox"));
    EXPECT_FALSE(a.startsWith('x'));
    EXPECT_FALSE(a.startsWith("x"));

    {
        auto b = a;
        EXPECT_TRUE(b.removePrefix(""));
        EXPECT_EQ("hello", b);
    }
  {
      auto b = a;
      EXPECT_TRUE(b.removePrefix("h"));
      EXPECT_EQ("ello", b);
  }
  {
      auto b = a;
      EXPECT_TRUE(b.removePrefix('h'));
      EXPECT_EQ("ello", b);
  }
  {
      auto b = a;
      EXPECT_TRUE(b.removePrefix("hello"));
      EXPECT_EQ("", b);
  }
  {
      auto b = a;
      EXPECT_FALSE(b.removePrefix("hellox"));
      EXPECT_EQ("hello", b);
  }
  {
      auto b = a;
      EXPECT_FALSE(b.removePrefix("x"));
      EXPECT_EQ("hello", b);
  }
  {
      auto b = a;
      EXPECT_FALSE(b.removePrefix('x'));
      EXPECT_EQ("hello", b);
  }
}

TEST(StringPiece, Suffix)
{
    StringPiece a("hello");
    EXPECT_TRUE(a.endsWith(""));
    EXPECT_TRUE(a.endsWith("o"));
    EXPECT_TRUE(a.endsWith('o'));
    EXPECT_TRUE(a.endsWith("hello"));
    EXPECT_FALSE(a.endsWith("xhello"));
    EXPECT_FALSE(a.endsWith("x"));
    EXPECT_FALSE(a.endsWith('x'));

    {
        auto b = a;
        EXPECT_TRUE(b.removeSuffix(""));
        EXPECT_EQ("hello", b);
    }
    {
        auto b = a;
        EXPECT_TRUE(b.removeSuffix("o"));
        EXPECT_EQ("hell", b);
    }
    {
        auto b = a;
        EXPECT_TRUE(b.removeSuffix('o'));
        EXPECT_EQ("hell", b);
    }
    {
        auto b = a;
        EXPECT_TRUE(b.removeSuffix("hello"));
        EXPECT_EQ("", b);
    }
    {
        auto b = a;
        EXPECT_FALSE(b.removeSuffix("xhello"));
        EXPECT_EQ("hello", b);
    }
    {
        auto b = a;
        EXPECT_FALSE(b.removeSuffix("x"));
        EXPECT_EQ("hello", b);
    }
    {
        auto b = a;
        EXPECT_FALSE(b.removeSuffix('x'));
        EXPECT_EQ("hello", b);
    }
}

TEST(StringPiece, PrefixEmpty)
{
    StringPiece a;
    EXPECT_TRUE(a.startsWith(""));
    EXPECT_FALSE(a.startsWith("a"));
    EXPECT_FALSE(a.startsWith('a'));
    EXPECT_TRUE(a.removePrefix(""));
    EXPECT_EQ("", a);
    EXPECT_FALSE(a.removePrefix("a"));
    EXPECT_EQ("", a);
    EXPECT_FALSE(a.removePrefix('a'));
    EXPECT_EQ("", a);
}

TEST(StringPiece, SuffixEmpty)
{
    StringPiece a;
    EXPECT_TRUE(a.endsWith(""));
    EXPECT_FALSE(a.endsWith("a"));
    EXPECT_FALSE(a.endsWith('a'));
    EXPECT_TRUE(a.removeSuffix(""));
    EXPECT_EQ("", a);
    EXPECT_FALSE(a.removeSuffix("a"));
    EXPECT_EQ("", a);
    EXPECT_FALSE(a.removeSuffix('a'));
    EXPECT_EQ("", a);
}

void split_step_with_process_noop(StringPiece) {}


template<class C>
void testRangeFunc(C&& x, size_t n)
{
    const auto& cx = x;
    // type, conversion checks
    Range<int*> r1 = range(std::forward<C>(x));
    Range<const int*> r2 = range(std::forward<C>(x));
    Range<const int*> r3 = range(cx);
    Range<const int*> r5 = range(std::move(cx));
#ifdef __GNUC__
    EXPECT_EQ(r1.begin(), &x[0]);
    EXPECT_EQ(r1.end(), &x[n]);
#endif
    EXPECT_EQ(n, r1.size());
    EXPECT_EQ(n, r2.size());
    EXPECT_EQ(n, r3.size());
    EXPECT_EQ(n, r5.size());
}

TEST(RangeFunc, Vector)
{
    std::vector<int> x;
    testRangeFunc(x, 0);
    x.push_back(2);
    testRangeFunc(x, 1);
    testRangeFunc(std::vector < int > {1, 2}, 2);
}

TEST(RangeFunc, Array)
{
    std::array<int, 3> x;
    testRangeFunc(x, 3);
}


std::string get_rand_str(
    int size, std::uniform_int_distribution<>& dist, std::mt19937& gen) 
{
    std::string ret(size, '\0');
    for (int i = 0; i < size; ++i) 
    {
        ret[i] = static_cast<char>(dist(gen));
    }

    return ret;
}

bool operator==(MutableStringPiece mp, StringPiece sp)
{
    return mp.compare(sp) == 0;
}

bool operator==(StringPiece sp, MutableStringPiece mp)
{
    return mp.compare(sp) == 0;
}

TEST(ReplaceAt, exhaustiveTest) 
{
    char input[] = "this is nice and long input";
    auto msp = MutableStringPiece(input);
    auto str = std::string(input);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist('a', 'z');

    for (int i = 0; i < 100; ++i) 
    {
        for (int j = 1; j <= msp.size(); ++j) 
        {
            auto replacement = get_rand_str(j, dist, gen);
            for (int pos = 0; pos < msp.size() - j; ++pos) 
            {
                msp.replaceAt(pos, replacement);
                str.replace(pos, replacement.size(), replacement);
                EXPECT_EQ(msp.compare(str), 0);
            }
        }
    }

    // too far
    EXPECT_EQ(msp.replaceAt(msp.size() - 2, StringPiece("meh")), false);
}
