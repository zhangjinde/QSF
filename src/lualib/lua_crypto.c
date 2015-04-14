// Copyright (C) 2014-2015 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <lua.h>
#include <lauxlib.h>
#include <openssl/md5.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <openssl/aes.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>


#define RSA_BITS        1024
#define MAX_RSA_BUF     16384

#define AES_HANDLE      "cryptAES*"
#define RSA_HANDLE      "cryptRSA*"

#define check_aes(L)   ((cryptAES*)luaL_checkudata(L, 1, AES_HANDLE))
#define check_rsa(L)   ((cryptRSA*)luaL_checkudata(L, 1, RSA_HANDLE))

typedef int(*RSAFuncType)(int, const uint8_t*, uint8_t*, RSA*, int);

typedef struct cryptAES
{
    uint8_t key[AES_BLOCK_SIZE];
    uint8_t iv[AES_BLOCK_SIZE];
    uint8_t buf[AES_BLOCK_SIZE * 512];  //16KB, big enough for our net packet
}cryptAES;

typedef struct cryptRSA
{
    RSA     ctx;
    uint8_t buf[MAX_RSA_BUF];  //16KB
}cryptRSA;


inline void HexDump(const void* data, size_t len, void* out, size_t outlen)
{
    assert(data && len > 0 && outlen >= len*2);
    static const char dict[] = "0123456789abcdef";
    const uint8_t* src = (const uint8_t*)(data);
    uint8_t* dst = (uint8_t*)out;
    for (size_t i = 0; i < len; i++)
    {
        uint8_t ch = src[i];
        *dst++ = dict[(ch & 0xF0) >> 4];
        *dst++ = dict[ch & 0x0F];
    }
}


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
    cryptAES* aes = check_aes(L);
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
        uint8_t block[AES_BLOCK_SIZE];
        memset(block, '\0', sizeof(block)); // padding zero
        memcpy(block, data, slop);
        AES_cbc_encrypt(block, dst, AES_BLOCK_SIZE, &ctx, iv, AES_ENCRYPT);
        dst += AES_BLOCK_SIZE;
    }
    lua_pushlstring(L, (const char*)aes->buf, dst - aes->buf);
    return 1;
}

static int crypto_aes_decrypt(lua_State* L)
{
    cryptAES* aes = check_aes(L);
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
    cryptRSA* rsa = (cryptRSA*)lua_newuserdata(L, sizeof(cryptRSA));
    memset(rsa, 0, sizeof(*rsa));
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
    int pubkey_len = BIO_pending(bio);
    char pubkey[1024];
    assert(pubkey_len < sizeof(pubkey));
    BIO_read(bio, pubkey, pubkey_len);

    // private key
    PEM_write_bio_RSAPrivateKey(bio, keypair, NULL, NULL, 0, NULL, NULL);
    int prikey_len = BIO_pending(bio);
    char prikey[1024];
    assert(prikey_len < sizeof(prikey));
    BIO_read(bio, prikey, prikey_len);
    BIO_free(bio);

    lua_pushlstring(L, pubkey, pubkey_len);
    lua_pushlstring(L, prikey, prikey_len);
    return 2;
}

static int crypto_rsa_set_pubkey(lua_State* L)
{
    cryptRSA* rsa = check_rsa(L);
    size_t len;
    const char* key = luaL_checklstring(L, 2, &len);
    BIO* bio = BIO_new(BIO_s_mem());
    BIO_write(bio, key, (int)len);
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
    cryptRSA* rsa = check_rsa(L);
    size_t len;
    const char* key = luaL_checklstring(L, 2, &len);
    BIO* bio = BIO_new(BIO_s_mem());
    BIO_write(bio, key, (int)len);
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

inline int do_rsa_encrypt(cryptRSA* rsa, RSAFuncType func, const uint8_t* src, int size)
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
    return (int)(dst - rsa->buf);
}

inline int do_rsa_decrypt(cryptRSA* rsa, RSAFuncType func, const uint8_t* src, int size)
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
    return (int)(dst - rsa->buf);
}

static int crypto_rsa_pub_encrypt(lua_State* L)
{
    cryptRSA* rsa = check_rsa(L);
    size_t len;
    const char* data = luaL_checklstring(L, 2, &len);
    int size = do_rsa_encrypt(rsa, RSA_public_encrypt, (const uint8_t*)data, (int)len);
    if (size > 0)
    {
        lua_pushlstring(L, (const char*)rsa->buf, size);
        return 1;
    }
    return 0;
}

static int crypto_rsa_encrypt(lua_State* L)
{
    cryptRSA* rsa = check_rsa(L);
    size_t len;
    const char* data = luaL_checklstring(L, 2, &len);
    int size = do_rsa_encrypt(rsa, RSA_private_encrypt, (const uint8_t*)data, (int)len);
    if (size > 0)
    {
        lua_pushlstring(L, (const char*)rsa->buf, size);
        return 1;
    }
    return 0;
}

static int crypto_rsa_pub_decrypt(lua_State* L)
{
    cryptRSA* rsa = check_rsa(L);
    size_t len;
    const char* data = luaL_checklstring(L, 2, &len);
    int size = do_rsa_decrypt(rsa, RSA_public_decrypt, (const uint8_t*)data, (int)len);
    if (size > 0)
    {
        lua_pushlstring(L, (const char*)rsa->buf, size);
        return 1;
    }
    return 0;
}

static int crypto_rsa_decrypt(lua_State* L)
{
    cryptRSA* rsa = check_rsa(L);
    size_t len;
    const char* data = luaL_checklstring(L, 2, &len);
    int size = do_rsa_decrypt(rsa, RSA_private_decrypt, (const uint8_t*)data, (int)len);
    if (size > 0)
    {
        lua_pushlstring(L, (const char*)rsa->buf, size);
        return 1;
    }
    return 0;
}

static int crypto_md5(lua_State* L)
{
    size_t len;
    const char* data = luaL_checklstring(L, 1, &len);
    uint8_t buffer[MD5_DIGEST_LENGTH];
    MD5((const uint8_t*)data, len, buffer);
    char hex[MD5_DIGEST_LENGTH * 2];
    HexDump(buffer, MD5_DIGEST_LENGTH, hex, sizeof(hex));
    lua_pushlstring(L, hex, sizeof(hex));
    return 1;
}

static int crypto_sha1(lua_State* L)
{
    size_t len = 0;
    const uint8_t* data = (const uint8_t*)luaL_checklstring(L, 1, &len);
    uint8_t buffer[SHA_DIGEST_LENGTH];
    SHA1(data, len, buffer);
    char hex[SHA_DIGEST_LENGTH * 2];
    HexDump(buffer, SHA_DIGEST_LENGTH, hex, sizeof(hex));
    lua_pushlstring(L, hex, sizeof(hex));
    return 1;
}

static int crypto_hmac_md5(lua_State* L)
{
    size_t key_len, size;
    const char* key = luaL_checklstring(L, 1, &key_len);
    const char* data = luaL_checklstring(L, 2, &size);
    const EVP_MD* evp_md = EVP_md5();
    unsigned int md_len = 0;
    uint8_t md[EVP_MAX_MD_SIZE];
    HMAC(evp_md, key, (int)key_len, (const uint8_t*)data, size, md, &md_len);
    char hex[EVP_MAX_MD_SIZE * 2];
    HexDump(md, md_len, hex, sizeof(hex));
    lua_pushlstring(L, hex, md_len*2);
    return 1;
}

static int crypto_hmac_sha1(lua_State* L)
{
    size_t key_len, size;
    const char* key = luaL_checklstring(L, 1, &key_len);
    const char* data = luaL_checklstring(L, 2, &size);
    const EVP_MD* evp_md = EVP_sha1();
    unsigned int md_len = 0;
    uint8_t md[EVP_MAX_MD_SIZE];
    HMAC(evp_md, key, (int)key_len, (const uint8_t*)data, size, md, &md_len);
    char hex[EVP_MAX_MD_SIZE * 2];
    HexDump(md, md_len, hex, sizeof(hex));
    lua_pushlstring(L, hex, md_len*2);
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

LUALIB_API int luaopen_crypto(lua_State* L)
{
    static const luaL_Reg lib[] =
    {
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
