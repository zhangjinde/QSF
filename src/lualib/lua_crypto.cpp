// Copyright (C) 2014 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#include <stdint.h>
#include <lua.hpp>
#include <openssl/md5.h>
#include <openssl/sha.h>


inline void BinaryToHex(const void* input, size_t inlen, 
                        const void* output, size_t outlen)
{
    static const char dict[] = "0123456789abcdef";
    const uint8_t* src = (const uint8_t*)input;
    uint8_t* dst = (uint8_t*)output;
    size_t pos = 0;
    for (size_t i = 0; i < inlen && pos < outlen; i++)
    {
        uint8_t ch = src[i];
        dst[pos++] = dict[(ch & 0xF0) >> 4];
        dst[pos++] = dict[ch & 0x0F];
    }
}

static int crypto_md5(lua_State* L)
{
    size_t len = 0;
    const uint8_t* data = (const uint8_t*)luaL_checklstring(L, 1, &len);
    uint8_t hash[MD5_DIGEST_LENGTH];
    MD5(data, len, hash);
    char hex[MD5_DIGEST_LENGTH * 2] = {'\0'};
    BinaryToHex(hash, sizeof(hash), hex, sizeof(hex));
    lua_pushlstring(L, hex, sizeof(hex));
    return 1;
}

static int crypto_sha1(lua_State* L)
{
    size_t len = 0;
    const uint8_t* data = (const uint8_t*)luaL_checklstring(L, 1, &len);
    uint8_t hash[SHA_DIGEST_LENGTH];
    SHA1(data, len, hash);
    char hex[SHA_DIGEST_LENGTH * 2] = { '\0' };
    BinaryToHex(hash, sizeof(hash), hex, sizeof(hex));
    lua_pushlstring(L, hex, sizeof(hex));
    return 1;
}

extern "C"
int luaopen_crypto(lua_State* L)
{
    static const luaL_Reg lib[] =
    {
        { "md5", crypto_md5 },
        { "sha1", crypto_sha1 },
        { NULL, NULL },
    };
    luaL_newlib(L, lib);
    return 1;
}
