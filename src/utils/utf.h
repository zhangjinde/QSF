// Copyright (C) 2014 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#pragma once

#ifdef _WIN32

#include <string>
#include <utility>

/**
 * Multibytes: GBK, BIG5, UTF-8
 *
 * UCS-2: UTF-16 in BMP(Basic Multilingual Plane)
 */

enum
{
    CodePage_GB2312 = 936,    
    CodePage_BIG5 = 950,
    CodePage_GB18030 = 54936,
    CodePage_UTF8 = 65001,
};


// Multibytes to UCS-2
bool Mbs2Wide(const std::string& strMbs,
              std::wstring* strWide,
              unsigned codepage,
              unsigned flags = 0);

// UCS-2 to multibytes
bool Wide2Mbs(const std::wstring& strWide,
              std::string* strMbs,
              unsigned codepage,
              unsigned flags = 0,
              bool useDefault = true);

// GB2312 to UCS-2
inline std::wstring AtoW(const std::string& strACP)
{
    std::wstring strWide;
    if (!Mbs2Wide(strACP, &strWide, CodePage_GB2312))
    {
        strWide.clear();
    }
    return std::move(strWide);
}

// UCS-2 to GB2312
inline std::string WtoA(const std::wstring& strWide)
{
    std::string strACP;
    if (!Wide2Mbs(strWide, &strACP, CodePage_GB2312))
    {
        strACP.clear();
    }
    return std::move(strACP);
}

// UCS-2 to UTF-8
inline std::string WtoU8(const std::wstring& strWide)
{
    std::string strUtf8;
    if (!Wide2Mbs(strWide, &strUtf8, CodePage_UTF8, 0, false))
    {
        strUtf8.clear();
    }
    return std::move(strUtf8);
}

// UTF-8 to UCS-2
inline std::wstring U8toW(const std::string& strUtf8)
{
    std::wstring strWide;
    if (!Mbs2Wide(strUtf8, &strWide, CodePage_UTF8))
    {
        strWide.clear();
    }
    return std::move(strWide);
}

// UTF-8 to GB2312
inline std::string U8toA(const std::string& strUtf8)
{
    return WtoA(U8toW(strUtf8));
}

// GB2312 to UTF-8
inline std::string AtoU8(const std::string& strGbk)
{
    return WtoA(AtoW(strGbk));
}

#endif // _WIN32
