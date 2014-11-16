#pragma once

#include <string>
#include <cstdlib>

inline std::string randString(size_t size)
{
    static const char alpha[] =
        "0123456789"
        "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    const size_t length = strlen(alpha);

    std::string s;
    s.resize(size);
    for (size_t i = 0; i < size; i++)
    {
        auto chosen = rand() % length;
        s[i] = alpha[chosen];
    }
    return s;
}
