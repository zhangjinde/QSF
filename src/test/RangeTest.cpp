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

namespace detail {

// declaration of functions in Range.cpp
size_t qfind_first_byte_of_memchr(const StringPiece& haystack,
                                  const StringPiece& needles);

size_t qfind_first_byte_of_byteset(const StringPiece& haystack,
                                   const StringPiece& needles);

}  // namespaces


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

    // find
    s.reset(foobarbaz, strlen(foobarbaz));
    EXPECT_EQ(s.find("bar"), 3);
    EXPECT_EQ(s.find("ba", 3), 3);
    EXPECT_EQ(s.find("ba", 4), 6);
    EXPECT_EQ(s.find("notfound"), StringPiece::npos);
    EXPECT_EQ(s.find("notfound", 1), StringPiece::npos);
    EXPECT_EQ(s.find("bar", 4), StringPiece::npos);  // starting position too far
    // starting pos that is obviously past the end -- This works for std::string
    EXPECT_EQ(s.toString().find("notfound", 55), StringPiece::npos);
    EXPECT_EQ(s.find("z", s.size()), StringPiece::npos);
    EXPECT_EQ(s.find("z", 55), StringPiece::npos);
    // empty needle
    EXPECT_EQ(s.find(""), std::string().find(""));
    EXPECT_EQ(s.find(""), 0);

    // single char finds
    EXPECT_EQ(s.find('b'), 3);
    EXPECT_EQ(s.find('b', 3), 3);
    EXPECT_EQ(s.find('b', 4), 6);
    EXPECT_EQ(s.find('o', 2), 2);
    EXPECT_EQ(s.find('y'), StringPiece::npos);
    EXPECT_EQ(s.find('y', 1), StringPiece::npos);
    EXPECT_EQ(s.find('o', 4), StringPiece::npos);  // starting position too far
    EXPECT_TRUE(s.contains('z'));
    // starting pos that is obviously past the end -- This works for std::string
    EXPECT_EQ(s.toString().find('y', 55), StringPiece::npos);
    EXPECT_EQ(s.find('z', s.size()), StringPiece::npos);
    EXPECT_EQ(s.find('z', 55), StringPiece::npos);
    // null char
    EXPECT_EQ(s.find('\0'), std::string().find('\0'));
    EXPECT_EQ(s.find('\0'), StringPiece::npos);
    EXPECT_FALSE(s.contains('\0'));

    // single char rfinds
    EXPECT_EQ(s.rfind('b'), 6);
    EXPECT_EQ(s.rfind('y'), StringPiece::npos);
    EXPECT_EQ(s.str().rfind('y'), StringPiece::npos);
    EXPECT_EQ(ByteRange(s).rfind('b'), 6);
    EXPECT_EQ(ByteRange(s).rfind('y'), StringPiece::npos);
    // null char
    EXPECT_EQ(s.rfind('\0'), s.str().rfind('\0'));
    EXPECT_EQ(s.rfind('\0'), StringPiece::npos);

    // find_first_of
    s.reset(foobarbaz, strlen(foobarbaz));
    EXPECT_EQ(s.find_first_of("bar"), 3);
    EXPECT_EQ(s.find_first_of("ba", 3), 3);
    EXPECT_EQ(s.find_first_of("ba", 4), 4);
    EXPECT_TRUE(s.contains("bar"));
    EXPECT_EQ(s.find_first_of("xyxy"), StringPiece::npos);
    EXPECT_EQ(s.find_first_of("xyxy", 1), StringPiece::npos);
    EXPECT_FALSE(s.contains("xyxy"));
    // starting position too far
    EXPECT_EQ(s.find_first_of("foo", 4), StringPiece::npos);
    // starting pos that is obviously past the end -- This works for std::string
    EXPECT_EQ(s.toString().find_first_of("xyxy", 55), StringPiece::npos);
    EXPECT_EQ(s.find_first_of("z", s.size()), StringPiece::npos);
    EXPECT_EQ(s.find_first_of("z", 55), StringPiece::npos);
    // empty needle. Note that this returns npos, while find() returns 0!
    EXPECT_EQ(s.find_first_of(""), std::string().find_first_of(""));
    EXPECT_EQ(s.find_first_of(""), StringPiece::npos);

    // single char find_first_ofs
    EXPECT_EQ(s.find_first_of('b'), 3);
    EXPECT_EQ(s.find_first_of('b', 3), 3);
    EXPECT_EQ(s.find_first_of('b', 4), 6);
    EXPECT_EQ(s.find_first_of('o', 2), 2);
    EXPECT_EQ(s.find_first_of('y'), StringPiece::npos);
    EXPECT_EQ(s.find_first_of('y', 1), StringPiece::npos);
    // starting position too far
    EXPECT_EQ(s.find_first_of('o', 4), StringPiece::npos);
    // starting pos that is obviously past the end -- This works for std::string
    EXPECT_EQ(s.toString().find_first_of('y', 55), StringPiece::npos);
    EXPECT_EQ(s.find_first_of('z', s.size()), StringPiece::npos);
    EXPECT_EQ(s.find_first_of('z', 55), StringPiece::npos);
    // null char
    EXPECT_EQ(s.find_first_of('\0'), std::string().find_first_of('\0'));
    EXPECT_EQ(s.find_first_of('\0'), StringPiece::npos);

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

TEST(StringPiece, split_step_char_delimiter)
{
    //              0         1         2
    //              012345678901234567890123456
    auto const s = "this is just  a test string";
    auto const e = std::next(s, std::strlen(s));
    EXPECT_EQ('\0', *e);

    StringPiece p(s);
    EXPECT_EQ(s, p.begin());
    EXPECT_EQ(e, p.end());

    auto x = p.split_step(' ');
    EXPECT_EQ(std::next(s, 5), p.begin());
    EXPECT_EQ(e, p.end());
    EXPECT_EQ("this", x);

    x = p.split_step(' ');
    EXPECT_EQ(std::next(s, 8), p.begin());
    EXPECT_EQ(e, p.end());
    EXPECT_EQ("is", x);

    x = p.split_step('u');
    EXPECT_EQ(std::next(s, 10), p.begin());
    EXPECT_EQ(e, p.end());
    EXPECT_EQ("j", x);

    x = p.split_step(' ');
    EXPECT_EQ(std::next(s, 13), p.begin());
    EXPECT_EQ(e, p.end());
    EXPECT_EQ("st", x);

    x = p.split_step(' ');
    EXPECT_EQ(std::next(s, 14), p.begin());
    EXPECT_EQ(e, p.end());
    EXPECT_EQ("", x);

    x = p.split_step(' ');
    EXPECT_EQ(std::next(s, 16), p.begin());
    EXPECT_EQ(e, p.end());
    EXPECT_EQ("a", x);

    x = p.split_step(' ');
    EXPECT_EQ(std::next(s, 21), p.begin());
    EXPECT_EQ(e, p.end());
    EXPECT_EQ("test", x);

    x = p.split_step(' ');
    EXPECT_EQ(e, p.begin());
    EXPECT_EQ(e, p.end());
    EXPECT_EQ("string", x);

    x = p.split_step(' ');
    EXPECT_EQ(e, p.begin());
    EXPECT_EQ(e, p.end());
    EXPECT_EQ("", x);
}

TEST(StringPiece, split_step_range_delimiter)
{
    //              0         1         2         3
    //              0123456789012345678901234567890123
    auto const s = "this  is  just    a   test  string";
    auto const e = std::next(s, std::strlen(s));
    EXPECT_EQ('\0', *e);

    StringPiece p(s);
    EXPECT_EQ(s, p.begin());
    EXPECT_EQ(e, p.end());

    auto x = p.split_step("  ");
    EXPECT_EQ(std::next(s, 6), p.begin());
    EXPECT_EQ(e, p.end());
    EXPECT_EQ("this", x);

    x = p.split_step("  ");
    EXPECT_EQ(std::next(s, 10), p.begin());
    EXPECT_EQ(e, p.end());
    EXPECT_EQ("is", x);

    x = p.split_step("u");
    EXPECT_EQ(std::next(s, 12), p.begin());
    EXPECT_EQ(e, p.end());
    EXPECT_EQ("j", x);

    x = p.split_step("  ");
    EXPECT_EQ(std::next(s, 16), p.begin());
    EXPECT_EQ(e, p.end());
    EXPECT_EQ("st", x);

    x = p.split_step("  ");
    EXPECT_EQ(std::next(s, 18), p.begin());
    EXPECT_EQ(e, p.end());
    EXPECT_EQ("", x);

    x = p.split_step("  ");
    EXPECT_EQ(std::next(s, 21), p.begin());
    EXPECT_EQ(e, p.end());
    EXPECT_EQ("a", x);

    x = p.split_step("  ");
    EXPECT_EQ(std::next(s, 28), p.begin());
    EXPECT_EQ(e, p.end());
    EXPECT_EQ(" test", x);

    x = p.split_step("  ");
    EXPECT_EQ(e, p.begin());
    EXPECT_EQ(e, p.end());
    EXPECT_EQ("string", x);

    x = p.split_step("  ");
    EXPECT_EQ(e, p.begin());
    EXPECT_EQ(e, p.end());
    EXPECT_EQ("", x);

    x = p.split_step(" ");
    EXPECT_EQ(e, p.begin());
    EXPECT_EQ(e, p.end());
    EXPECT_EQ("", x);
}

void split_step_with_process_noop(StringPiece) {}

TEST(StringPiece, split_step_with_process_char_delimiter)
{
    //              0         1         2
    //              012345678901234567890123456
    auto const s = "this is just  a test string";
    auto const e = std::next(s, std::strlen(s));
    EXPECT_EQ('\0', *e);

    StringPiece p(s);
    EXPECT_EQ(s, p.begin());
    EXPECT_EQ(e, p.end());

    auto fn = [&](StringPiece x)
    {
        EXPECT_EQ(std::next(s, 5), p.begin());
        EXPECT_EQ(e, p.end());
        EXPECT_EQ("this", x);
        return 1;
    };
    EXPECT_EQ(1, (p.split_step(' ', fn)));

    auto fn2 = [&](StringPiece x)
    {
        EXPECT_EQ(std::next(s, 8), p.begin());
        EXPECT_EQ(e, p.end());
        EXPECT_EQ("is", x);
        return 2;
    };
    EXPECT_EQ(2, (p.split_step(' ', fn2)));

    auto fn3 = [&](StringPiece x)
    {
        EXPECT_EQ(std::next(s, 10), p.begin());
        EXPECT_EQ(e, p.end());
        EXPECT_EQ("j", x);
        return 3;
    };
    EXPECT_EQ(3, (p.split_step('u', fn3)));

    auto fn4 = [&](StringPiece x)
    {
        EXPECT_EQ(std::next(s, 13), p.begin());
        EXPECT_EQ(e, p.end());
        EXPECT_EQ("st", x);
        return 4;
    };
    EXPECT_EQ(4, (p.split_step(' ', fn4)));

    auto fn5 = [&](StringPiece x)
    {
        EXPECT_EQ(std::next(s, 14), p.begin());
        EXPECT_EQ(e, p.end());
        EXPECT_EQ("", x);
        return 5;
    };
    EXPECT_EQ(5, (p.split_step(' ', fn5)));

    auto fn6 = [&](StringPiece x)
    {
        EXPECT_EQ(std::next(s, 16), p.begin());
        EXPECT_EQ(e, p.end());
        EXPECT_EQ("a", x);
        return 6;
    };
    EXPECT_EQ(6, (p.split_step(' ', fn6)));

    auto fn7 = [&](StringPiece x)
    {
        EXPECT_EQ(std::next(s, 21), p.begin());
        EXPECT_EQ(e, p.end());
        EXPECT_EQ("test", x);
        return 7;
    };
    EXPECT_EQ(7, (p.split_step(' ', fn7)));

    auto fn8 = [&](StringPiece x)
    {
        EXPECT_EQ(e, p.begin());
        EXPECT_EQ(e, p.end());
        EXPECT_EQ("string", x);
        return 8;
    };
    EXPECT_EQ(8, (p.split_step(' ', fn8)));

    auto fn9 = [&](StringPiece x)
    {
        EXPECT_EQ(e, p.begin());
        EXPECT_EQ(e, p.end());
        EXPECT_EQ("", x);
        return 9;
    };
    EXPECT_EQ(9, (p.split_step(' ', fn9)));

    EXPECT_TRUE((std::is_same <
        void,
        decltype(p.split_step(' ', split_step_with_process_noop))
    > ::value));

    EXPECT_NO_THROW(p.split_step(' ', split_step_with_process_noop));
}

TEST(StringPiece, split_step_with_process_range_delimiter)
{
    //              0         1         2         3
    //              0123456789012345678901234567890123
    auto const s = "this  is  just    a   test  string";
    auto const e = std::next(s, std::strlen(s));
    EXPECT_EQ('\0', *e);

    StringPiece p(s);
    EXPECT_EQ(s, p.begin());
    EXPECT_EQ(e, p.end());

    auto fn1 = [&](StringPiece x)
    {
        EXPECT_EQ(std::next(s, 6), p.begin());
        EXPECT_EQ(e, p.end());
        EXPECT_EQ("this", x);
        return 1;
    };
    EXPECT_EQ(1, (p.split_step("  ", fn1)));

    auto fn2 = [&](StringPiece x)
    {
        EXPECT_EQ(std::next(s, 10), p.begin());
        EXPECT_EQ(e, p.end());
        EXPECT_EQ("is", x);
        return 2;
    };
    EXPECT_EQ(2, (p.split_step("  ", fn2)));

    auto fn3 = [&](StringPiece x)
    {
        EXPECT_EQ(std::next(s, 12), p.begin());
        EXPECT_EQ(e, p.end());
        EXPECT_EQ("j", x);
        return 3;
    };
    EXPECT_EQ(3, (p.split_step("u", fn3)));

    auto fn4 = [&](StringPiece x)
    {
        EXPECT_EQ(std::next(s, 16), p.begin());
        EXPECT_EQ(e, p.end());
        EXPECT_EQ("st", x);
        return 4;
    };
    EXPECT_EQ(4, (p.split_step("  ", fn4)));

    auto fn5 = [&](StringPiece x)
    {
        EXPECT_EQ(std::next(s, 18), p.begin());
        EXPECT_EQ(e, p.end());
        EXPECT_EQ("", x);
        return 5;
    };
    EXPECT_EQ(5, (p.split_step("  ", fn5)));

    auto fn6 = [&](StringPiece x)
    {
        EXPECT_EQ(std::next(s, 21), p.begin());
        EXPECT_EQ(e, p.end());
        EXPECT_EQ("a", x);
        return 6;
    };
    EXPECT_EQ(6, (p.split_step("  ", fn6)));

    auto fn7 = [&](StringPiece x)
    {
        EXPECT_EQ(std::next(s, 28), p.begin());
        EXPECT_EQ(e, p.end());
        EXPECT_EQ(" test", x);
        return 7;
    };
    EXPECT_EQ(7, (p.split_step("  ", fn7)));

    auto fn8 = [&](StringPiece x)
    {
        EXPECT_EQ(e, p.begin());
        EXPECT_EQ(e, p.end());
        EXPECT_EQ("string", x);
        return 8;
    };
    EXPECT_EQ(8, (p.split_step("  ", fn8)));

    auto fn9 = [&](StringPiece x)
    {
        EXPECT_EQ(e, p.begin());
        EXPECT_EQ(e, p.end());
        EXPECT_EQ("", x);
        return 9;
    };
    EXPECT_EQ(9, (p.split_step("  ", fn9)));

    auto fn10 = [&](StringPiece x)
    {
        EXPECT_EQ(e, p.begin());
        EXPECT_EQ(e, p.end());
        EXPECT_EQ("", x);
        return 10;
    };
    EXPECT_EQ(10, (p.split_step("  ", fn10)));

    EXPECT_TRUE((std::is_same <
        void,
        decltype(p.split_step(' ', split_step_with_process_noop))
    > ::value));

    EXPECT_NO_THROW(p.split_step(' ', split_step_with_process_noop));
}


TEST(StringPiece, split_step_with_process_char_delimiter_additional_args) {
    //              0         1         2
    //              012345678901234567890123456
    auto const s = "this is just  a test string";
    auto const e = std::next(s, std::strlen(s));
    auto const delimiter = ' ';
    EXPECT_EQ('\0', *e);

    StringPiece p(s);
    EXPECT_EQ(s, p.begin());
    EXPECT_EQ(e, p.end());
    EXPECT_EQ(s, p);

    auto const functor = [](
        StringPiece s,
        StringPiece expected
        ) 
    {
        EXPECT_EQ(expected, s);
        return expected;
    };

    auto const checker = [&](StringPiece expected) 
    {
        EXPECT_EQ(expected, p.split_step(delimiter, functor, expected));
    };

    checker("this");
    checker("is");
    checker("just");
    checker("");
    checker("a");
    checker("test");
    checker("string");
    checker("");
    checker("");

    EXPECT_TRUE(p.empty());
}

TEST(StringPiece, split_step_with_process_range_delimiter_additional_args) {
    //              0         1         2         3
    //              0123456789012345678901234567890123
    auto const s = "this  is  just    a   test  string";
    auto const e = std::next(s, std::strlen(s));
    auto const delimiter = "  ";
    EXPECT_EQ('\0', *e);

    StringPiece p(s);
    EXPECT_EQ(s, p.begin());
    EXPECT_EQ(e, p.end());
    EXPECT_EQ(s, p);

    auto const functor = [](
        StringPiece s,
        StringPiece expected
        ) 
    {
        EXPECT_EQ(expected, s);
        return expected;
    };

    auto const checker = [&](StringPiece expected) 
    {
        EXPECT_EQ(expected, p.split_step(delimiter, functor, expected));
    };

    checker("this");
    checker("is");
    checker("just");
    checker("");
    checker("a");
    checker(" test");
    checker("string");
    checker("");
    checker("");

    EXPECT_TRUE(p.empty());
}


TEST(qfind, UInt32_Ranges)
{
    vector<uint32_t> a({ 1, 2, 3, 260, 5 });
    vector<uint32_t> b({ 2, 3, 4 });

    auto a_range = Range<const uint32_t*>(&a[0], a.size());
    auto b_range = Range<const uint32_t*>(&b[0], b.size());

    EXPECT_EQ(qfind(a_range, b_range), string::npos);

    a[3] = 4;
    EXPECT_EQ(qfind(a_range, b_range), 1);
}

template <typename NeedleFinder>
class NeedleFinderTest : public ::testing::Test
{
public:
    static size_t find_first_byte_of(StringPiece haystack, StringPiece needles)
    {
        return NeedleFinder::find_first_byte_of(haystack, needles);
    }
};

struct SseNeedleFinder
{
    static size_t find_first_byte_of(StringPiece haystack, StringPiece needles)
    {
        // This will only use the SSE version if it is supported on this CPU
        // (selected using ifunc).
        return detail::qfind_first_byte_of(haystack, needles);
    }
};

struct NoSseNeedleFinder
{
    static size_t find_first_byte_of(StringPiece haystack, StringPiece needles)
    {
        return detail::qfind_first_byte_of_nosse(haystack, needles);
    }
};

struct MemchrNeedleFinder
{
    static size_t find_first_byte_of(StringPiece haystack, StringPiece needles)
    {
        return detail::qfind_first_byte_of_memchr(haystack, needles);
    }
};

struct ByteSetNeedleFinder
{
    static size_t find_first_byte_of(StringPiece haystack, StringPiece needles)
    {
        return detail::qfind_first_byte_of_byteset(haystack, needles);
    }
};

typedef ::testing::Types < SseNeedleFinder, NoSseNeedleFinder, MemchrNeedleFinder,
    ByteSetNeedleFinder > NeedleFinders;
TYPED_TEST_CASE(NeedleFinderTest, NeedleFinders);

TYPED_TEST(NeedleFinderTest, Null)
{
    { // null characters in the string
        string s(10, char(0));
        s[5] = 'b';
        string delims("abc");
        EXPECT_EQ(5, this->find_first_byte_of(s, delims));
    }
    { // null characters in delim
        string s("abc");
        string delims(10, char(0));
        delims[3] = 'c';
        delims[7] = 'b';
        EXPECT_EQ(1, this->find_first_byte_of(s, delims));
    }
    { // range not terminated by null character
        string buf = "abcdefghijklmnopqrstuvwxyz";
        StringPiece s(buf.data() + 5, 3);
        StringPiece delims("z");
        EXPECT_EQ(string::npos, this->find_first_byte_of(s, delims));
    }
}

TYPED_TEST(NeedleFinderTest, DelimDuplicates) 
{
    string delims(1000, 'b');
    EXPECT_EQ(1, this->find_first_byte_of("abc", delims));
    EXPECT_EQ(string::npos, this->find_first_byte_of("ac", delims));
}

TYPED_TEST(NeedleFinderTest, Empty) 
{
    string a = "abc";
    string b = "";
    EXPECT_EQ(string::npos, this->find_first_byte_of(a, b));
    EXPECT_EQ(string::npos, this->find_first_byte_of(b, a));
    EXPECT_EQ(string::npos, this->find_first_byte_of(b, b));
}

TYPED_TEST(NeedleFinderTest, Unaligned)
{
    // works correctly even if input buffers are not 16-byte aligned
    string s = "0123456789ABCDEFGH";
    for (int i = 0; i < s.size(); ++i)
    {
        StringPiece a(s.c_str() + i);
        for (int j = 0; j < s.size(); ++j)
        {
            StringPiece b(s.c_str() + j);
            EXPECT_EQ((i > j) ? 0 : j - i, this->find_first_byte_of(a, b));
        }
    }
}

// for some algorithms (specifically those that create a set of needles),
// we check for the edge-case of _all_ possible needles being sought.
TYPED_TEST(NeedleFinderTest, Needles256)
{
    string needles;
    const auto minValue = std::numeric_limits<StringPiece::value_type>::min();
    const auto maxValue = std::numeric_limits<StringPiece::value_type>::max();
    // make the size ~big to avoid any edge-case branches for tiny haystacks
    const int haystackSize = 50;
    for (int i = minValue; i <= maxValue; i++)   // <=
    {
        needles.push_back(i);
    }
    EXPECT_EQ(StringPiece::npos, this->find_first_byte_of("", needles));
    for (int i = minValue; i <= maxValue; i++)
    {
        EXPECT_EQ(0, this->find_first_byte_of(string(haystackSize, i), needles));
    }

    needles.append("these are redundant characters");
    EXPECT_EQ(StringPiece::npos, this->find_first_byte_of("", needles));
    for (int i = minValue; i <= maxValue; i++)
    {
        EXPECT_EQ(0, this->find_first_byte_of(string(haystackSize, i), needles));
    }
}

TYPED_TEST(NeedleFinderTest, Base)
{
    for (int i = 0; i < 32; ++i)
    {
        for (int j = 0; j < 32; ++j)
        {
            string s = string(i, 'X') + "abca" + string(i, 'X');
            string delims = string(j, 'Y') + "a" + string(j, 'Y');
            EXPECT_EQ(i, this->find_first_byte_of(s, delims));
        }
    }
}

TEST(NonConstTest, StringPiece)
{
    std::string hello("hello");
    MutableStringPiece sp(&hello.front(), hello.size());
    sp[0] = 'x';
    EXPECT_EQ("xello", hello);
    {
        StringPiece s(sp);
        EXPECT_EQ("xello", s);
    }
  {
      ByteRange r1(sp);
      MutableByteRange r2(sp);
  }
}

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

TEST(ReplaceAll, basicTest) 
{
    char input[] = "this is nice and long input";
    auto orig = std::string(input);
    auto msp = MutableStringPiece(input);

    EXPECT_EQ(msp.replaceAll("is", "si"), 2);
    EXPECT_EQ("thsi si nice and long input", msp);
    EXPECT_EQ(msp.replaceAll("si", "is"), 2);
    EXPECT_EQ(msp, orig);

    EXPECT_EQ(msp.replaceAll("abcd", "efgh"), 0); // nothing to replace
    EXPECT_EQ(msp, orig);

    // at the very beginning
    EXPECT_EQ(msp.replaceAll("this", "siht"), 1);
    EXPECT_EQ("siht is nice and long input", msp);
    EXPECT_EQ(msp.replaceAll("siht", "this"), 1);
    EXPECT_EQ(msp, orig);

    // at the very end
    EXPECT_EQ(msp.replaceAll("input", "soput"), 1);
    EXPECT_EQ("this is nice and long soput", msp);
    EXPECT_EQ(msp.replaceAll("soput", "input"), 1);
    EXPECT_EQ(msp, orig);

    // all spaces
    EXPECT_EQ(msp.replaceAll(" ", "@"), 5);
    EXPECT_EQ("this@is@nice@and@long@input", msp);
    EXPECT_EQ(msp.replaceAll("@", " "), 5);
    EXPECT_EQ(msp, orig);
}

TEST(ReplaceAll, randomTest) 
{
    char input[] = "abcdefghijklmnoprstuwqz"; // no pattern repeata inside
    auto orig = std::string(input);
    auto msp = MutableStringPiece(input);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist('A', 'Z');

    for (int i = 0; i < 100; ++i) 
    {
        for (int j = 1; j <= orig.size(); ++j) 
        {
            auto replacement = get_rand_str(j, dist, gen);
            for (int pos = 0; pos < msp.size() - j; ++pos) 
            {
                auto piece = orig.substr(pos, j);
                EXPECT_EQ(msp.replaceAll(piece, replacement), 1);
                EXPECT_EQ(msp.find(replacement), pos);
                EXPECT_EQ(msp.replaceAll(replacement, piece), 1);
                EXPECT_EQ(msp, orig);
            }
        }
    }
}

TEST(ReplaceAll, BadArg) 
{
    int count = 0;
    auto fst = "longer";
    auto snd = "small";
    char input[] = "meh meh meh";
    auto all = MutableStringPiece(input);

    try 
    {
        all.replaceAll(fst, snd);
    }
    catch (std::invalid_argument&) 
    {
        ++count;
    }

    try 
    {
        all.replaceAll(snd, fst);
    }
    catch (std::invalid_argument&) 
    {
        ++count;
    }

    EXPECT_EQ(count, 2);
}
