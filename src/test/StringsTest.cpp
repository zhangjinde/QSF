#include <gtest/gtest.h>
#include <cinttypes>
#include <vector>
#include "Benchmark.h"
#include "core/Strings.h"

using namespace std;


TEST(Strings, stringPrintf)
{
    // basic
    EXPECT_EQ("abc", stringPrintf("%s", "abc"));
    EXPECT_EQ("abc", stringPrintf("%sbc", "a"));
    EXPECT_EQ("abc", stringPrintf("a%sc", "b"));
    EXPECT_EQ("abc", stringPrintf("ab%s", "c"));
    EXPECT_EQ("abc", stringPrintf("abc"));
    
    // numeric formats
    EXPECT_EQ("12", stringPrintf("%d", 12));
    EXPECT_EQ("5000000000", stringPrintf("%" PRIi64, 5000000000UL));
    EXPECT_EQ("5000000000", stringPrintf("%" PRIu64, 5000000000L));
    EXPECT_EQ("-5000000000", stringPrintf("%" PRIi64, -5000000000L));
    EXPECT_EQ("-1", stringPrintf("%d", 0xffffffff));
    EXPECT_EQ("-1", stringPrintf("%" PRIi64, 0xffffffffffffffff));
    EXPECT_EQ("-1", stringPrintf("%" PRIi64, 0xffffffffffffffffUL));

    EXPECT_EQ("7.7", stringPrintf("%1.1f", 7.7));
    EXPECT_EQ("7.7", stringPrintf("%1.1lf", 7.7));
#ifdef _MSC_VER
    EXPECT_EQ("7.70000000000000020", stringPrintf("%.17f", 7.7));
    EXPECT_EQ("7.70000000000000020", stringPrintf("%.17lf", 7.7));
#else    
    EXPECT_EQ("7.70000000000000018", stringPrintf("%.17f", 7.7));
    EXPECT_EQ("7.70000000000000018", stringPrintf("%.17lf", 7.7));
#endif    
}

TEST(Strings, stringAppendf)
{
    string s;
    stringAppendf(&s, "a%s", "b");
    stringAppendf(&s, "%c", 'c');
    EXPECT_EQ(s, "abc");
    stringAppendf(&s, " %d", 123);
    EXPECT_EQ(s, "abc 123");
}

TEST(Strings, stringPrintfVariousSizes)
{
    // Test a wide variety of output sizes
    for (int i = 0; i < 100; ++i)
    {
        string expected(i + 1, 'a');
        EXPECT_EQ("X" + expected + "X", stringPrintf("X%sX", expected.c_str()));
    }

    EXPECT_EQ("abc12345678910111213141516171819202122232425xyz",
        stringPrintf("abc%d%d%d%d%d%d%d%d%d%d%d%d%d%d"
        "%d%d%d%d%d%d%d%d%d%d%dxyz",
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
        17, 18, 19, 20, 21, 22, 23, 24, 25));
}


TEST(Strings, oldStringPrintfTests)
{
    EXPECT_EQ(string("a/b/c/d"),
        stringPrintf("%s/%s/%s/%s", "a", "b", "c", "d"));

    EXPECT_EQ(string("    5    10"),
        stringPrintf("%5d %5d", 5, 10));

    // check printing w/ a big buffer
    for (int size = (1 << 8); size <= (1 << 15); size <<= 1) {
        string a(size, 'z');
        string b = stringPrintf("%s", a.c_str());
        EXPECT_EQ(a.size(), b.size());
    }
}

TEST(Strings, oldStringAppendf)
{
    string s = "hello";
    stringAppendf(&s, "%s/%s/%s/%s", "a", "b", "c", "d");
    EXPECT_EQ(string("helloa/b/c/d"), s);
}


template<template<class, class> class VectorType>
void splitTest() 
{
    VectorType<string, std::allocator<string> > parts;

    split(',', "a,b,c", parts);
    EXPECT_EQ(parts.size(), 3);
    EXPECT_EQ(parts[0], "a");
    EXPECT_EQ(parts[1], "b");
    EXPECT_EQ(parts[2], "c");
    parts.clear();

    split(',', StringPiece("a,b,c"), parts);
    EXPECT_EQ(parts.size(), 3);
    EXPECT_EQ(parts[0], "a");
    EXPECT_EQ(parts[1], "b");
    EXPECT_EQ(parts[2], "c");
    parts.clear();

    split(',', string("a,b,c"), parts);
    EXPECT_EQ(parts.size(), 3);
    EXPECT_EQ(parts[0], "a");
    EXPECT_EQ(parts[1], "b");
    EXPECT_EQ(parts[2], "c");
    parts.clear();

    split(',', "a,,c", parts);
    EXPECT_EQ(parts.size(), 3);
    EXPECT_EQ(parts[0], "a");
    EXPECT_EQ(parts[1], "");
    EXPECT_EQ(parts[2], "c");
    parts.clear();

    split(',', string("a,,c"), parts);
    EXPECT_EQ(parts.size(), 3);
    EXPECT_EQ(parts[0], "a");
    EXPECT_EQ(parts[1], "");
    EXPECT_EQ(parts[2], "c");
    parts.clear();

    split(',', "a,,c", parts, true);
    EXPECT_EQ(parts.size(), 2);
    EXPECT_EQ(parts[0], "a");
    EXPECT_EQ(parts[1], "c");
    parts.clear();

    split(',', string("a,,c"), parts, true);
    EXPECT_EQ(parts.size(), 2);
    EXPECT_EQ(parts[0], "a");
    EXPECT_EQ(parts[1], "c");
    parts.clear();

    split(',', string(",,a,,c,,,"), parts, true);
    EXPECT_EQ(parts.size(), 2);
    EXPECT_EQ(parts[0], "a");
    EXPECT_EQ(parts[1], "c");
    parts.clear();

    // test multiple split w/o clear
    split(',', ",,a,,c,,,", parts, true);
    EXPECT_EQ(parts.size(), 2);
    EXPECT_EQ(parts[0], "a");
    EXPECT_EQ(parts[1], "c");
    split(',', ",,a,,c,,,", parts, true);
    EXPECT_EQ(parts.size(), 4);
    EXPECT_EQ(parts[2], "a");
    EXPECT_EQ(parts[3], "c");
    parts.clear();

    // test splits that with multi-line delimiter
    split("ab", "dabcabkdbkab", parts, true);
    EXPECT_EQ(parts.size(), 3);
    EXPECT_EQ(parts[0], "d");
    EXPECT_EQ(parts[1], "c");
    EXPECT_EQ(parts[2], "kdbk");
    parts.clear();

    string orig = "ab2342asdfv~~!";
    split("", orig, parts, true);
    EXPECT_EQ(parts.size(), 1);
    EXPECT_EQ(parts[0], orig);
    parts.clear();

    split("452x;o38asfsajsdlfdf.j", "asfds", parts, true);
    EXPECT_EQ(parts.size(), 1);
    EXPECT_EQ(parts[0], "asfds");
    parts.clear();

    split("a", "", parts, true);
    EXPECT_EQ(parts.size(), 0);
    parts.clear();

    split("a", "", parts);
    EXPECT_EQ(parts.size(), 1);
    EXPECT_EQ(parts[0], "");
    parts.clear();

    split("a", StringPiece(), parts, true);
    EXPECT_EQ(parts.size(), 0);
    parts.clear();

    split("a", StringPiece(), parts);
    EXPECT_EQ(parts.size(), 1);
    EXPECT_EQ(parts[0], "");
    parts.clear();

    split("a", "abcdefg", parts, true);
    EXPECT_EQ(parts.size(), 1);
    EXPECT_EQ(parts[0], "bcdefg");
    parts.clear();

    orig = "All, , your base, are , , belong to us";
    split(", ", orig, parts, true);
    EXPECT_EQ(parts.size(), 4);
    EXPECT_EQ(parts[0], "All");
    EXPECT_EQ(parts[1], "your base");
    EXPECT_EQ(parts[2], "are ");
    EXPECT_EQ(parts[3], "belong to us");
    parts.clear();
    split(", ", orig, parts);
    EXPECT_EQ(parts.size(), 6);
    EXPECT_EQ(parts[0], "All");
    EXPECT_EQ(parts[1], "");
    EXPECT_EQ(parts[2], "your base");
    EXPECT_EQ(parts[3], "are ");
    EXPECT_EQ(parts[4], "");
    EXPECT_EQ(parts[5], "belong to us");
    parts.clear();

    orig = ", Facebook, rul,es!, ";
    split(", ", orig, parts, true);
    EXPECT_EQ(parts.size(), 2);
    EXPECT_EQ(parts[0], "Facebook");
    EXPECT_EQ(parts[1], "rul,es!");
    parts.clear();
    split(", ", orig, parts);
    EXPECT_EQ(parts.size(), 4);
    EXPECT_EQ(parts[0], "");
    EXPECT_EQ(parts[1], "Facebook");
    EXPECT_EQ(parts[2], "rul,es!");
    EXPECT_EQ(parts[3], "");
}

template<template<class, class> class VectorType>
void piecesTest() 
{
    VectorType<StringPiece, std::allocator<StringPiece> > pieces;
    VectorType<StringPiece, std::allocator<StringPiece> > pieces2;

    split(',', "a,b,c", pieces);
    EXPECT_EQ(pieces.size(), 3);
    EXPECT_EQ(pieces[0], "a");
    EXPECT_EQ(pieces[1], "b");
    EXPECT_EQ(pieces[2], "c");

    pieces.clear();

    split(',', "a,,c", pieces);
    EXPECT_EQ(pieces.size(), 3);
    EXPECT_EQ(pieces[0], "a");
    EXPECT_EQ(pieces[1], "");
    EXPECT_EQ(pieces[2], "c");
    pieces.clear();

    split(',', "a,,c", pieces, true);
    EXPECT_EQ(pieces.size(), 2);
    EXPECT_EQ(pieces[0], "a");
    EXPECT_EQ(pieces[1], "c");
    pieces.clear();

    split(',', ",,a,,c,,,", pieces, true);
    EXPECT_EQ(pieces.size(), 2);
    EXPECT_EQ(pieces[0], "a");
    EXPECT_EQ(pieces[1], "c");
    pieces.clear();

    // test multiple split w/o clear
    split(',', ",,a,,c,,,", pieces, true);
    EXPECT_EQ(pieces.size(), 2);
    EXPECT_EQ(pieces[0], "a");
    EXPECT_EQ(pieces[1], "c");
    split(',', ",,a,,c,,,", pieces, true);
    EXPECT_EQ(pieces.size(), 4);
    EXPECT_EQ(pieces[2], "a");
    EXPECT_EQ(pieces[3], "c");
    pieces.clear();

    // test multiple split rounds
    split(",", "a_b,c_d", pieces);
    EXPECT_EQ(pieces.size(), 2);
    EXPECT_EQ(pieces[0], "a_b");
    EXPECT_EQ(pieces[1], "c_d");
    split("_", pieces[0], pieces2);
    EXPECT_EQ(pieces2.size(), 2);
    EXPECT_EQ(pieces2[0], "a");
    EXPECT_EQ(pieces2[1], "b");
    pieces2.clear();
    split("_", pieces[1], pieces2);
    EXPECT_EQ(pieces2.size(), 2);
    EXPECT_EQ(pieces2[0], "c");
    EXPECT_EQ(pieces2[1], "d");
    pieces.clear();
    pieces2.clear();

    // test splits that with multi-line delimiter
    split("ab", "dabcabkdbkab", pieces, true);
    EXPECT_EQ(pieces.size(), 3);
    EXPECT_EQ(pieces[0], "d");
    EXPECT_EQ(pieces[1], "c");
    EXPECT_EQ(pieces[2], "kdbk");
    pieces.clear();

    string orig = "ab2342asdfv~~!";
    split("", orig.c_str(), pieces, true);
    EXPECT_EQ(pieces.size(), 1);
    EXPECT_EQ(pieces[0], orig);
    pieces.clear();

    split("452x;o38asfsajsdlfdf.j", "asfds", pieces, true);
    EXPECT_EQ(pieces.size(), 1);
    EXPECT_EQ(pieces[0], "asfds");
    pieces.clear();

    split("a", "", pieces, true);
    EXPECT_EQ(pieces.size(), 0);
    pieces.clear();

    split("a", "", pieces);
    EXPECT_EQ(pieces.size(), 1);
    EXPECT_EQ(pieces[0], "");
    pieces.clear();

    split("a", "abcdefg", pieces, true);
    EXPECT_EQ(pieces.size(), 1);
    EXPECT_EQ(pieces[0], "bcdefg");
    pieces.clear();

    orig = "All, , your base, are , , belong to us";
    split(", ", orig, pieces, true);
    EXPECT_EQ(pieces.size(), 4);
    EXPECT_EQ(pieces[0], "All");
    EXPECT_EQ(pieces[1], "your base");
    EXPECT_EQ(pieces[2], "are ");
    EXPECT_EQ(pieces[3], "belong to us");
    pieces.clear();
    split(", ", orig, pieces);
    EXPECT_EQ(pieces.size(), 6);
    EXPECT_EQ(pieces[0], "All");
    EXPECT_EQ(pieces[1], "");
    EXPECT_EQ(pieces[2], "your base");
    EXPECT_EQ(pieces[3], "are ");
    EXPECT_EQ(pieces[4], "");
    EXPECT_EQ(pieces[5], "belong to us");
    pieces.clear();

    orig = ", Facebook, rul,es!, ";
    split(", ", orig, pieces, true);
    EXPECT_EQ(pieces.size(), 2);
    EXPECT_EQ(pieces[0], "Facebook");
    EXPECT_EQ(pieces[1], "rul,es!");
    pieces.clear();
    split(", ", orig, pieces);
    EXPECT_EQ(pieces.size(), 4);
    EXPECT_EQ(pieces[0], "");
    EXPECT_EQ(pieces[1], "Facebook");
    EXPECT_EQ(pieces[2], "rul,es!");
    EXPECT_EQ(pieces[3], "");
    pieces.clear();

    const char* str = "a,b";
    split(',', StringPiece(str), pieces);
    EXPECT_EQ(pieces.size(), 2);
    EXPECT_EQ(pieces[0], "a");
    EXPECT_EQ(pieces[1], "b");
    EXPECT_EQ(pieces[0].start(), str);
    EXPECT_EQ(pieces[1].start(), str + 2);

    std::set<StringPiece> unique;
    splitTo<StringPiece>(":", "asd:bsd:asd:asd:bsd:csd::asd",
        std::inserter(unique, unique.begin()), true);
    EXPECT_EQ(unique.size(), 3);
    if (unique.size() == 3) 
    {
        EXPECT_EQ(*unique.begin(), "asd");
        EXPECT_EQ(*--unique.end(), "csd");
    }
}

TEST(Strings, splitVector) 
{
    splitTest<std::vector>();
    piecesTest<std::vector>();
}


TEST(Split, stdStringFixed) 
{
    std::string a, b, c, d;

    EXPECT_TRUE(split<false>('.', "a.b.c.d", a, b, c, d));
    EXPECT_TRUE(split<false>('.', "a.b.c", a, b, c));
    EXPECT_TRUE(split<false>('.', "a.b", a, b));
    EXPECT_TRUE(split<false>('.', "a", a));

    EXPECT_TRUE(split('.', "a.b.c.d", a, b, c, d));
    EXPECT_TRUE(split('.', "a.b.c", a, b, c));
    EXPECT_TRUE(split('.', "a.b", a, b));
    EXPECT_TRUE(split('.', "a", a));

    EXPECT_TRUE(split<false>('.', "a.b.c", a, b, c));
    EXPECT_EQ("a", a);
    EXPECT_EQ("b", b);
    EXPECT_EQ("c", c);
    EXPECT_FALSE(split<false>('.', "a.b", a, b, c));
    EXPECT_TRUE(split<false>('.', "a.b.c", a, b));
    EXPECT_EQ("a", a);
    EXPECT_EQ("b.c", b);

    EXPECT_TRUE(split('.', "a.b.c", a, b, c));
    EXPECT_EQ("a", a);
    EXPECT_EQ("b", b);
    EXPECT_EQ("c", c);
    EXPECT_FALSE(split('.', "a.b.c", a, b));
    EXPECT_FALSE(split('.', "a.b", a, b, c));

    EXPECT_TRUE(split<false>('.', "a.b", a, b));
    EXPECT_EQ("a", a);
    EXPECT_EQ("b", b);
    EXPECT_FALSE(split<false>('.', "a", a, b));
    EXPECT_TRUE(split<false>('.', "a.b", a));
    EXPECT_EQ("a.b", a);

    EXPECT_TRUE(split('.', "a.b", a, b));
    EXPECT_EQ("a", a);
    EXPECT_EQ("b", b);
    EXPECT_FALSE(split('.', "a", a, b));
    EXPECT_FALSE(split('.', "a.b", a));
}


TEST(Strings, splitFixed) 
{
    StringPiece a, b, c, d;

    EXPECT_TRUE(split<false>('.', "a.b.c.d", a, b, c, d));
    EXPECT_TRUE(split<false>('.', "a.b.c", a, b, c));
    EXPECT_TRUE(split<false>('.', "a.b", a, b));
    EXPECT_TRUE(split<false>('.', "a", a));

    EXPECT_TRUE(split('.', "a.b.c.d", a, b, c, d));
    EXPECT_TRUE(split('.', "a.b.c", a, b, c));
    EXPECT_TRUE(split('.', "a.b", a, b));
    EXPECT_TRUE(split('.', "a", a));

    EXPECT_TRUE(split<false>('.', "a.b.c", a, b, c));
    EXPECT_EQ("a", a);
    EXPECT_EQ("b", b);
    EXPECT_EQ("c", c);
    EXPECT_FALSE(split<false>('.', "a.b", a, b, c));
    EXPECT_TRUE(split<false>('.', "a.b.c", a, b));
    EXPECT_EQ("a", a);
    EXPECT_EQ("b.c", b);

    EXPECT_TRUE(split('.', "a.b.c", a, b, c));
    EXPECT_EQ("a", a);
    EXPECT_EQ("b", b);
    EXPECT_EQ("c", c);
    EXPECT_FALSE(split('.', "a.b.c", a, b));
    EXPECT_FALSE(split('.', "a.b", a, b, c));

    EXPECT_TRUE(split<false>('.', "a.b", a, b));
    EXPECT_EQ("a", a);
    EXPECT_EQ("b", b);
    EXPECT_FALSE(split<false>('.', "a", a, b));
    EXPECT_TRUE(split<false>('.', "a.b", a));
    EXPECT_EQ("a.b", a);

    EXPECT_TRUE(split('.', "a.b", a, b));
    EXPECT_EQ("a", a);
    EXPECT_EQ("b", b);
    EXPECT_FALSE(split('.', "a", a, b));
    EXPECT_FALSE(split('.', "a.b", a));
}


TEST(Strings, SplitFixedConvert)
{
    StringPiece a, d;
    int b;
    double c;

    EXPECT_TRUE(split(':', "a:13:14.7:b", a, b, c, d));
    EXPECT_EQ("a", a);
    EXPECT_EQ(13, b);
    EXPECT_NEAR(14.7, c, 1e-10);
    EXPECT_EQ("b", d);

    EXPECT_TRUE(split<false>(':', "b:14:15.3:c", a, b, c, d));
    EXPECT_EQ("b", a);
    EXPECT_EQ(14, b);
    EXPECT_NEAR(15.3, c, 1e-10);
    EXPECT_EQ("c", d);

    EXPECT_FALSE(split(':', "a:13:14.7:b", a, b, d));

    EXPECT_TRUE(split<false>(':', "a:13:14.7:b", a, b, d));
    EXPECT_EQ("a", a);
    EXPECT_EQ(13, b);
    EXPECT_EQ("14.7:b", d);

    EXPECT_ANY_THROW(split<false>(':', "a:13:14.7:b", a, b, c));
}


TEST(Strings, join) 
{
    string output;

    std::vector<int> empty = {};
    join(":", empty, output);
    EXPECT_TRUE(output.empty());

    vector<string> input1 = { "1", "23", "456", "" };
    join(':', input1, output);
    EXPECT_EQ(output, "1:23:456:");
    output = join(':', input1);
    EXPECT_EQ(output, "1:23:456:");

    auto input2 = { 1, 23, 456 };
    join("-*-", input2, output);
    EXPECT_EQ(output, "1-*-23-*-456");
    output = join("-*-", input2);
    EXPECT_EQ(output, "1-*-23-*-456");

    auto input3 = { 'f', 'a', 'c', 'e', 'b', 'o', 'o', 'k' };
    join("", input3, output);
    EXPECT_EQ(output, "facebook");

    vector<string> input4 = { "", "f", "a", "c", "e", "b", "o", "o", "k", "" };
    join("_", input4, output);
    EXPECT_EQ(output, "_f_a_c_e_b_o_o_k_");
}

static double pow2(int exponent) 
{
    return double(int64_t(1) << exponent);
}

struct PrettyTestCase
{
    std::string prettyString;
    double realValue;
    PrettyType prettyType;
};


PrettyTestCase prettyTestCases[] =
{   
    { string("85.3 ms"), 85.3e-3, PRETTY_TIME },
    { string("85.3 us"), 85.3e-6, PRETTY_TIME },
    { string("85.3 ns"), 85.3e-9, PRETTY_TIME },
    { string("85.3 ps"), 85.3e-12, PRETTY_TIME },
#ifdef _MSC_VER
    { string("8.53e+007 s "), 85.3e6, PRETTY_TIME },
    { string("8.53e-014 s "), 85.3e-15, PRETTY_TIME },    
#else    
    { string("8.53e+07 s "), 85.3e6, PRETTY_TIME },
    { string("8.53e-14 s "), 85.3e-15, PRETTY_TIME },
#endif     


    { string("0 s "), 0, PRETTY_TIME },
    { string("1 s "), 1.0, PRETTY_TIME },
    { string("1 ms"), 1.0e-3, PRETTY_TIME },
    { string("1 us"), 1.0e-6, PRETTY_TIME },
    { string("1 ns"), 1.0e-9, PRETTY_TIME },
    { string("1 ps"), 1.0e-12, PRETTY_TIME },

    // check bytes printing
    { string("853 B "), 853., PRETTY_BYTES },
    { string("833 kB"), 853.e3, PRETTY_BYTES },
    { string("813.5 MB"), 853.e6, PRETTY_BYTES },
    { string("7.944 GB"), 8.53e9, PRETTY_BYTES },
    { string("794.4 GB"), 853.e9, PRETTY_BYTES },
    { string("775.8 TB"), 853.e12, PRETTY_BYTES },

    { string("0 B "), 0, PRETTY_BYTES },
    { string("1 B "), pow2(0), PRETTY_BYTES },
    { string("1 kB"), pow2(10), PRETTY_BYTES },
    { string("1 MB"), pow2(20), PRETTY_BYTES },
    { string("1 GB"), pow2(30), PRETTY_BYTES },
    { string("1 TB"), pow2(40), PRETTY_BYTES },

    { string("853 B  "), 853., PRETTY_BYTES_IEC },
    { string("833 KiB"), 853.e3, PRETTY_BYTES_IEC },
    { string("813.5 MiB"), 853.e6, PRETTY_BYTES_IEC },
    { string("7.944 GiB"), 8.53e9, PRETTY_BYTES_IEC },
    { string("794.4 GiB"), 853.e9, PRETTY_BYTES_IEC },
    { string("775.8 TiB"), 853.e12, PRETTY_BYTES_IEC },

    { string("0 B  "), 0, PRETTY_BYTES_IEC },
    { string("1 B  "), pow2(0), PRETTY_BYTES_IEC },
    { string("1 KiB"), pow2(10), PRETTY_BYTES_IEC },
    { string("1 MiB"), pow2(20), PRETTY_BYTES_IEC },
    { string("1 GiB"), pow2(30), PRETTY_BYTES_IEC },
    { string("1 TiB"), pow2(40), PRETTY_BYTES_IEC },

    // check bytes metric printing
    { string("853 B "), 853., PRETTY_BYTES_METRIC },
    { string("853 kB"), 853.e3, PRETTY_BYTES_METRIC },
    { string("853 MB"), 853.e6, PRETTY_BYTES_METRIC },
    { string("8.53 GB"), 8.53e9, PRETTY_BYTES_METRIC },
    { string("853 GB"), 853.e9, PRETTY_BYTES_METRIC },
    { string("853 TB"), 853.e12, PRETTY_BYTES_METRIC },

    { string("0 B "), 0, PRETTY_BYTES_METRIC },
    { string("1 B "), 1.0, PRETTY_BYTES_METRIC },
    { string("1 kB"), 1.0e+3, PRETTY_BYTES_METRIC },
    { string("1 MB"), 1.0e+6, PRETTY_BYTES_METRIC },

    { string("1 GB"), 1.0e+9, PRETTY_BYTES_METRIC },
    { string("1 TB"), 1.0e+12, PRETTY_BYTES_METRIC },

    // check metric-units (powers of 1000) printing
    { string("853  "), 853., PRETTY_UNITS_METRIC },
    { string("853 k"), 853.e3, PRETTY_UNITS_METRIC },
    { string("853 M"), 853.e6, PRETTY_UNITS_METRIC },
    { string("8.53 bil"), 8.53e9, PRETTY_UNITS_METRIC },
    { string("853 bil"), 853.e9, PRETTY_UNITS_METRIC },
    { string("853 tril"), 853.e12, PRETTY_UNITS_METRIC },

    // check binary-units (powers of 1024) printing
    { string("0  "), 0, PRETTY_UNITS_BINARY },
    { string("1  "), pow2(0), PRETTY_UNITS_BINARY },
    { string("1 k"), pow2(10), PRETTY_UNITS_BINARY },
    { string("1 M"), pow2(20), PRETTY_UNITS_BINARY },
    { string("1 G"), pow2(30), PRETTY_UNITS_BINARY },
    { string("1 T"), pow2(40), PRETTY_UNITS_BINARY },

    { string("1023  "), pow2(10) - 1, PRETTY_UNITS_BINARY },
    { string("1024 k"), pow2(20) - 1, PRETTY_UNITS_BINARY },
    { string("1024 M"), pow2(30) - 1, PRETTY_UNITS_BINARY },
    { string("1024 G"), pow2(40) - 1, PRETTY_UNITS_BINARY },

    { string("0   "), 0, PRETTY_UNITS_BINARY_IEC },
    { string("1   "), pow2(0), PRETTY_UNITS_BINARY_IEC },
    { string("1 Ki"), pow2(10), PRETTY_UNITS_BINARY_IEC },
    { string("1 Mi"), pow2(20), PRETTY_UNITS_BINARY_IEC },
    { string("1 Gi"), pow2(30), PRETTY_UNITS_BINARY_IEC },
    { string("1 Ti"), pow2(40), PRETTY_UNITS_BINARY_IEC },

    { string("1023   "), pow2(10) - 1, PRETTY_UNITS_BINARY_IEC },
    { string("1024 Ki"), pow2(20) - 1, PRETTY_UNITS_BINARY_IEC },
    { string("1024 Mi"), pow2(30) - 1, PRETTY_UNITS_BINARY_IEC },
    { string("1024 Gi"), pow2(40) - 1, PRETTY_UNITS_BINARY_IEC },

    //check border SI cases

    { string("1 Y"), 1e24, PRETTY_SI },
    { string("10 Y"), 1e25, PRETTY_SI },
    { string("1 y"), 1e-24, PRETTY_SI },
    { string("10 y"), 1e-23, PRETTY_SI },

    // check that negative values work
    { string("-85.3 s "), -85.3, PRETTY_TIME },
    { string("-85.3 ms"), -85.3e-3, PRETTY_TIME },
    { string("-85.3 us"), -85.3e-6, PRETTY_TIME },
    { string("-85.3 ns"), -85.3e-9, PRETTY_TIME },
    // end of test
    { string("endoftest"), 0, PRETTY_NUM_TYPES }
};

TEST(Strings, PrettyPrint) 
{
    for (int i = 0; prettyTestCases[i].prettyType != PRETTY_NUM_TYPES; ++i)
    {
        const PrettyTestCase& prettyTest = prettyTestCases[i];
        EXPECT_EQ(prettyTest.prettyString,
            prettyPrint(prettyTest.realValue, prettyTest.prettyType));
    }
}


TEST(Strings, PrettyToDouble)
{
    // check manually created tests
    for (int i = 0; prettyTestCases[i].prettyType != PRETTY_NUM_TYPES; ++i)
    {
        PrettyTestCase testCase = prettyTestCases[i];
        PrettyType formatType = testCase.prettyType;
        double x = testCase.realValue;
        std::string testString = testCase.prettyString;
        double recoveredX = 0.0;
        EXPECT_NO_THROW(recoveredX = prettyToDouble(testString, formatType));
        double relativeError = fabs(x) < 1e-5 ? (x - recoveredX) :
            (x - recoveredX) / x;
        EXPECT_NEAR(0, relativeError, 1e-3);
    }

    // checks for compatibility with prettyPrint over the whole parameter space
    for (int i = 0; i < PRETTY_NUM_TYPES; ++i)
    {
        PrettyType formatType = static_cast<PrettyType>(i);
        for (double x = 1e-18; x < 1e40; x *= 1.9)
        {
            bool addSpace = static_cast<PrettyType> (i) == PRETTY_SI;
            for (int it = 0; it < 2; ++it, addSpace = true)
            {
                double recoveredX = 0.0;
                EXPECT_NO_THROW(recoveredX = prettyToDouble(
                    prettyPrint(x, formatType, addSpace), formatType));
                double relativeError = (x - recoveredX) / x;
                EXPECT_NEAR(0, relativeError, 1e-3);
            }
        }
    }

    // check for incorrect values
    EXPECT_ANY_THROW(prettyToDouble("10Mx", PRETTY_SI));
    EXPECT_ANY_THROW(prettyToDouble("10 Mx", PRETTY_SI));
    EXPECT_ANY_THROW(prettyToDouble("10 M x", PRETTY_SI));

    StringPiece testString = "10Mx";
    EXPECT_DOUBLE_EQ(prettyToDouble(&testString, PRETTY_UNITS_METRIC), 10e6);
    EXPECT_EQ(testString, "x");
}



//////////////////////////////////////////////////////////////////////

BENCHMARK(splitOnSingleChar, iters) 
{
    static const std::string line = "one:two:three:four";
    for (unsigned i = 0; i < iters << 4; ++i) 
    {
        std::vector<StringPiece> pieces;
        split(':', line, pieces);
    }
}

BENCHMARK(splitOnSingleCharFixed, iters) 
{
    static const std::string line = "one:two:three:four";
    for (unsigned i = 0; i < iters << 4; ++i)
    {
        StringPiece a, b, c, d;
        split(':', line, a, b, c, d);
    }
}

BENCHMARK(splitOnSingleCharFixedAllowExtra, iters) 
{
    static const std::string line = "one:two:three:four";
    for (unsigned i = 0; i < iters << 4; ++i)
    {
        StringPiece a, b, c, d;
        split<false>(':', line, a, b, c, d);
    }
}

BENCHMARK(splitStr, iters) 
{
    static const std::string line = "one-*-two-*-three-*-four";
    for (unsigned i = 0; i < iters << 4; ++i)
    {
        std::vector<StringPiece> pieces;
        split("-*-", line, pieces);
    }
}

BENCHMARK(splitStrFixed, iters) 
{
    static const std::string line = "one-*-two-*-three-*-four";
    for (unsigned i = 0; i < iters << 4; ++i)
    {
        StringPiece a, b, c, d;
        split("-*-", line, a, b, c, d);
    }
}

//BENCHMARK(boost_splitOnSingleChar, iters) 
//{
//    static const std::string line = "one:two:three:four";
//    bool(*pred)(char) = [](char c) -> bool { return c == ':'; };
//    for (int i = 0; i < iters << 4; ++i) 
//    {
//        std::vector<boost::iterator_range<std::string::const_iterator> > pieces;
//        boost::split(pieces, line, pred);
//    }
//}

BENCHMARK(joinCharStr, iters) 
{
    static const std::vector<std::string> input = 
    {
        "one", "two", "three", "four", "five", "six", "seven" 
    };
    for (unsigned i = 0; i < iters << 4; ++i)
    {
        std::string output;
        join(':', input, output);
    }
}

BENCHMARK(joinStrStr, iters) 
{
    static const std::vector<std::string> input = 
    {
        "one", "two", "three", "four", "five", "six", "seven" 
    };
    for (unsigned i = 0; i < iters << 4; ++i)
    {
        std::string output;
        join(":", input, output);
    }
}

BENCHMARK(joinInt, iters) 
{
    static const auto input = 
    {
        123, 456, 78910, 1112, 1314, 151, 61718 
    };
    for (unsigned i = 0; i < iters << 4; ++i)
    {
        std::string output;
        join(":", input, output);
    }
}
