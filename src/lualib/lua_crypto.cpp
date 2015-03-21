// Copyright (C) 2014 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#include <stdint.h>
#include <string>
#include <lua.hpp>
#include <openssl/md5.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <openssl/aes.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include "core/Checksum.h"
#include "Hexdump.h"

#define RSA_BITS        1024
#define MAX_RSA_BUF     16384

#define AES_HANDLE      "cryptAES*"
#define RSA_HANDLE      "cryptRSA*"

#define check_aes(L)   ((cryptAES*)luaL_checkudata(L, 1, AES_HANDLE))
#define check_rsa(L)   ((cryptRSA*)luaL_checkudata(L, 1, RSA_HANDLE))

struct cryptAES
{
    uint8_t key[AES_BLOCK_SIZE];
    uint8_t iv[AES_BLOCK_SIZE];
    uint8_t buf[AES_BLOCK_SIZE * 512];  //16KB, big enough for our net packet
};

struct cryptRSA
{
    RSA     ctx;
    uint8_t buf[MAX_RSA_BUF];  //16KB
};

typedef int(*rsa_func)(int, const uint8_t*, uint8_t*, rsa_st*, int);

static int crypto_aes_new(lua_State* L)
{
    size_t key_len, iv_len;
    const char* key = luaL_checklstring(L, 1, &key_len);
    if (key_len != AES_BLOCK_SIZE)
    {
        return luaL_error(L, "invalid AES key size: %d", key_len);
    }
    const char* iv = lua_tolstring(L, 2, &iv_len);
    if (iv && iv_len != AES_BLOCK_SIZE)
    {
        return luaL_error(L, "invalid AES init vector size: %d", key_len);
    }
    cryptAES* ptr = (cryptAES*)lua_newuserdata(L, sizeof(cryptAES));
    memcpy(ptr->key, key, key_len);
    if (iv)
    {
        memcpy(ptr->iv, iv, iv_len);
    }
    else
    {
        memset(ptr->iv, 0, AES_BLOCK_SIZE);
    }
    luaL_getmetatable(L, AES_HANDLE);
    lua_setmetatable(L, -2);
    return 1;
}

static int crypto_aes_encrypt(lua_State* L)
{
    auto aes = check_aes(L);
    size_t size;
    const uint8_t* data = (const uint8_t*)luaL_checklstring(L, 2, &size);
    if (size > sizeof(aes->buf))
    {
        return luaL_error(L, "encrypt data too long: %d", size);
    }

    AES_KEY ctx;
    uint8_t iv[AES_BLOCK_SIZE];
    memcpy(iv, aes->iv, sizeof(iv));
    AES_set_encrypt_key(aes->key, 128, &ctx);
    uint8_t* dst = aes->buf;
    const int slop = size & (AES_BLOCK_SIZE - 1);
    for (size_t i = 0; i < size / AES_BLOCK_SIZE; ++i)
    {
        AES_cbc_encrypt(data, dst, AES_BLOCK_SIZE, &ctx, iv, AES_ENCRYPT);
        dst += AES_BLOCK_SIZE;
        data += AES_BLOCK_SIZE;
    }
    if (slop > 0) // padding
    {
        uint8_t block[AES_BLOCK_SIZE] = {};
        memcpy(block, data, slop);
        AES_cbc_encrypt(block, dst, AES_BLOCK_SIZE, &ctx, iv, AES_ENCRYPT);
        dst += AES_BLOCK_SIZE;
    }
    lua_pushlstring(L, (const char*)aes->buf, dst - aes->buf);
    return 1;
}

static int crypto_aes_decrypt(lua_State* L)
{
    auto aes = check_aes(L);
    size_t size;
    const uint8_t* data = (const uint8_t*)luaL_checklstring(L, 2, &size);
    luaL_argcheck(L, (size % AES_BLOCK_SIZE == 0), 2, "invalid block size");

    AES_KEY ctx;
    uint8_t iv[AES_BLOCK_SIZE];
    AES_set_decrypt_key(aes->key, 128, &ctx);
    memcpy(iv, aes->iv, sizeof(iv));

    uint8_t* dst = aes->buf;
    for (size_t i = 0; i < size / AES_BLOCK_SIZE; i++)
    {
        AES_cbc_encrypt(data, dst, AES_BLOCK_SIZE, &ctx, iv, AES_DECRYPT);
        data += AES_BLOCK_SIZE;
        dst += AES_BLOCK_SIZE;
    }
    // remove padding zero
    while (*(dst-1) == '\0')
        --dst;
    lua_pushlstring(L, (const char*)aes->buf, dst - aes->buf);
    return 1;
}

static int crypto_rsa_new(lua_State* L)
{
    cryptRSA* ptr = (cryptRSA*)lua_newuserdata(L, sizeof(cryptRSA));
    luaL_getmetatable(L, RSA_HANDLE);
    lua_setmetatable(L, -2);
    return 1;
}

static int crypto_rsa_gen_keypair(lua_State* L)
{
    RSA* keypair = RSA_new();
    BIGNUM* e = BN_new();
    BN_set_word(e, RSA_3);
    int r = RSA_generate_key_ex(keypair, RSA_BITS, e, NULL);
    BN_free(e);
    if (r != 1)
    {
        return luaL_error(L, "generate key failed.");
    }

    // public key
    BIO* bio = BIO_new(BIO_s_mem());
    PEM_write_bio_RSAPublicKey(bio, keypair);
    size_t length = BIO_pending(bio);
    std::string pubkey(length, '\0');
    BIO_read(bio, (char*)pubkey.data(), length);

    // private key
    PEM_write_bio_RSAPrivateKey(bio, keypair, NULL, NULL, 0, NULL, NULL);
    length = BIO_pending(bio);
    std::string prikey(length, '\0');
    BIO_read(bio, (char*)prikey.data(), length);
    BIO_free(bio);

    lua_pushlstring(L, pubkey.data(), pubkey.size());
    lua_pushlstring(L, prikey.data(), prikey.size());
    return 2;
}

static int crypto_rsa_set_pubkey(lua_State* L)
{
    auto rsa = check_rsa(L);
    size_t len;
    const char* key = luaL_checklstring(L, 2, &len);
    BIO* bio = BIO_new(BIO_s_mem());
    BIO_write(bio, key, len);
    RSA* ctx = PEM_read_bio_RSAPublicKey(bio, NULL, NULL, NULL);
    BIO_free(bio);
    if (ctx)
    {
        memcpy(&rsa->ctx, ctx, sizeof(*ctx));
    }
    else
    {
        return luaL_error(L, "set public key failed.");
    }
    return 0;
}

static int crypto_rsa_set_key(lua_State* L)
{
    auto rsa = check_rsa(L);
    size_t len;
    const char* key = luaL_checklstring(L, 2, &len);
    BIO* bio = BIO_new(BIO_s_mem());
    BIO_write(bio, key, len);
    RSA* ctx = PEM_read_bio_RSAPrivateKey(bio, NULL, NULL, NULL);
    BIO_free(bio);
    if (ctx)
    {
        memcpy(&rsa->ctx, ctx, sizeof(*ctx));
    }
    else
    {
        return luaL_error(L, "set private key failed.");
    }
    return 0;
}

inline int do_rsa_encrypt(cryptRSA* rsa, rsa_func func, const uint8_t* src, int size)
{
    assert(rsa && func && src && size > 0);
    uint8_t* dst = rsa->buf;
    const int modulus_size = RSA_size(&rsa->ctx);
    const int block_size = modulus_size - 11;
    int need_size = modulus_size * ((size / block_size) + ((size % block_size) > 0 ? 1 : 0));
    if (need_size > MAX_RSA_BUF)
    {
        return -2;
    }
    while (size > 0)
    {
        int len = (size > block_size ? block_size : size);
        int r = func(len, src, dst, &rsa->ctx, RSA_PKCS1_PADDING);
        if (r > 0)
        {
            size -= len;
            src += len;
            dst += r;
        }
        else
        {
            return -1;
        }
    }
    return dst - rsa->buf;
}

inline int do_rsa_decrypt(cryptRSA* rsa, rsa_func func, const uint8_t* src, int size)
{
    assert(rsa && func && src && size > 0);
    const int modulus_size = RSA_size(&rsa->ctx);
    if (size % modulus_size != 0)
    {
        return -3;
    }
    int need_size = (modulus_size - 11) * (size / modulus_size);
    if (need_size > MAX_RSA_BUF)
    {
        return -2;
    }
    uint8_t* dst = rsa->buf;
    while (size > 0)
    {
        int len = (size > modulus_size ? modulus_size : size);
        int r = func(len, src, dst, &rsa->ctx, RSA_PKCS1_PADDING);
        if (r > 0)
        {
            size -= len;
            src += len;
            dst += r;
        }
        else
        {
            return -1;
        }
    }
    return dst - rsa->buf;
}

static int crypto_rsa_pub_encrypt(lua_State* L)
{
    auto rsa = check_rsa(L);
    size_t len;
    const char* data = luaL_checklstring(L, 2, &len);
    int size = do_rsa_encrypt(rsa, RSA_public_encrypt, (const uint8_t*)data, len);
    if (size > 0)
    {
        lua_pushlstring(L, (const char*)rsa->buf, size);
        return 1;
    }
    return 0;
}

static int crypto_rsa_encrypt(lua_State* L)
{
    auto rsa = check_rsa(L);
    size_t len;
    const char* data = luaL_checklstring(L, 2, &len);
    int size = do_rsa_encrypt(rsa, RSA_private_encrypt, (const uint8_t*)data, len);
    if (size > 0)
    {
        lua_pushlstring(L, (const char*)rsa->buf, size);
        return 1;
    }
    return 0;
}

static int crypto_rsa_pub_decrypt(lua_State* L)
{
    auto rsa = check_rsa(L);
    size_t len;
    const char* data = luaL_checklstring(L, 2, &len);
    int size = do_rsa_decrypt(rsa, RSA_public_decrypt, (const uint8_t*)data, len);
    if (size > 0)
    {
        lua_pushlstring(L, (const char*)rsa->buf, size);
        return 1;
    }
    return 0;
}

static int crypto_rsa_decrypt(lua_State* L)
{
    auto rsa = check_rsa(L);
    size_t len;
    const char* data = luaL_checklstring(L, 2, &len);
    int size = do_rsa_decrypt(rsa, RSA_private_decrypt, (const uint8_t*)data, len);
    if (size > 0)
    {
        lua_pushlstring(L, (const char*)rsa->buf, size);
        return 1;
    }
    return 0;
}

static int crypto_crc32c(lua_State* L)
{
    size_t len;
    const char* data = luaL_checklstring(L, 1, &len);
    uint32_t init_value = (uint32_t)luaL_optinteger(L, 2, 0);
    uint32_t checksum = crc32c(data, len, init_value);
    lua_pushinteger(L, checksum);
    return 1;
}

static int crypto_md5(lua_State* L)
{
    size_t len;
    const char* data = luaL_checklstring(L, 1, &len);
    uint8_t buffer[MD5_DIGEST_LENGTH];
    MD5((const uint8_t*)data, len, buffer);
    std::string hex = BinaryToHex(buffer, MD5_DIGEST_LENGTH);
    lua_pushlstring(L, hex.c_str(), hex.size());
    return 1;
}

static int crypto_sha1(lua_State* L)
{
    size_t len = 0;
    const uint8_t* data = (const uint8_t*)luaL_checklstring(L, 1, &len);
    uint8_t buffer[SHA_DIGEST_LENGTH];
    SHA1(data, len, buffer);
    std::string hex = BinaryToHex(buffer, SHA_DIGEST_LENGTH);
    lua_pushlstring(L, hex.c_str(), hex.size());
    return 1;
}

static int crypto_hmac_md5(lua_State* L)
{
    size_t key_len, md_len, size;
    const char* key = luaL_checklstring(L, 1, &key_len);
    const char* data = luaL_checklstring(L, 2, &size);
    auto evp_md = EVP_md5();
    uint8_t md[EVP_MAX_MD_SIZE];
    HMAC(evp_md, key, key_len, (const uint8_t*)data, size, md, &md_len);
    auto str = BinaryToHex(md, md_len);
    lua_pushlstring(L, str.data(), str.size());
    return 1;
}

static int crypto_hmac_sha1(lua_State* L)
{
    size_t key_len, md_len, size;
    const char* key = luaL_checklstring(L, 1, &key_len);
    const char* data = luaL_checklstring(L, 2, &size);
    auto evp_md = EVP_sha1();
    uint8_t md[EVP_MAX_MD_SIZE];
    HMAC(evp_md, key, key_len, (const uint8_t*)data, size, md, &md_len);
    auto str = BinaryToHex(md, md_len);
    lua_pushlstring(L, str.data(), str.size());
    return 1;
}

static void create_meta(lua_State* L, const char* name, const luaL_Reg* methods)
{
    assert(L && name && methods);
    if (luaL_newmetatable(L, name))
    {
        lua_pushvalue(L, -1);
        lua_setfield(L, -2, "__index");
        luaL_setfuncs(L, methods, 0);
        lua_pushliteral(L, "__metatable");
        lua_pushliteral(L, "cannot access this metatable");
        lua_settable(L, -3);
        lua_pop(L, 1);  /* pop new metatable */
        return;
    }
    luaL_error(L, "`%s` already registered.", name);
}

static void make_meta(lua_State* L)
{
    static const luaL_Reg aes_lib[] =
    {
        { "encrypt", crypto_aes_encrypt },
        { "decrypt", crypto_aes_decrypt },
        { NULL, NULL },
    };

    static const luaL_Reg rsa_lib[] =
    {
        { "set_pubkey", crypto_rsa_set_pubkey },
        { "set_key", crypto_rsa_set_key },
        { "pub_encrypt", crypto_rsa_pub_encrypt },
        { "pub_decrypt", crypto_rsa_pub_decrypt },
        { "encrypt", crypto_rsa_encrypt },
        { "decrypt", crypto_rsa_decrypt },
        { NULL, NULL },
    };
    create_meta(L, AES_HANDLE, aes_lib);
    create_meta(L, RSA_HANDLE, rsa_lib);
}

extern "C" 
int luaopen_crypto(lua_State* L)
{
    static const luaL_Reg lib[] =
    {
        { "crc32c", crypto_crc32c },
        { "md5", crypto_md5 },
        { "sha1", crypto_sha1 },
        { "hmac_md5", crypto_hmac_md5 },
        { "hmac_sha1", crypto_hmac_sha1 },
        { "new_aes", crypto_aes_new },
        { "new_rsa", crypto_rsa_new },
        { "gen_rsa_keypair", crypto_rsa_gen_keypair },
        { NULL, NULL },
    };

    luaL_newlib(L, lib);
    make_meta(L);
    return 1;
}
