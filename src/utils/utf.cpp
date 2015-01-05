// Copyright (C) 2014 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#include "utf.h"

#ifdef _WIN32
#include <assert.h>
#include <windows.h>

bool Mbs2Wide(const std::string& strMbs,
              std::wstring* strWide,
              unsigned codepage,
              unsigned flags /* = 0 */)
{
    assert(strWide != NULL);
    int count = MultiByteToWideChar(codepage, 0, strMbs.data(), strMbs.length(), NULL, 0);
    if (count > 0)
    {
        strWide->resize(count);
        count = MultiByteToWideChar(codepage, flags, strMbs.data(), strMbs.length(),
            const_cast<wchar_t*>(strWide->data()), strWide->length());
    }
    return count > 0;
}


bool Wide2Mbs(const std::wstring& strWide,
              std::string* strMbs,
              unsigned codepage,
              unsigned flags /* = 0 */,
              bool useDefault /* = true */)
{
    assert(strMbs != NULL);
    BOOL bUseDefault = FALSE;
    int count = WideCharToMultiByte(codepage, 0, strWide.data(), strWide.length(), NULL, 0, NULL, NULL);
    if (count > 0)
    {
        strMbs->resize(count);
        BOOL* pUseDefault = (useDefault ? &bUseDefault : NULL);
        count = WideCharToMultiByte(codepage, flags, strWide.data(), strWide.length(),
            const_cast<char*>(strMbs->data()), strMbs->length(), NULL, pUseDefault);
    }
    return (count > 0 && !bUseDefault);
}

#endif // _WIN32
