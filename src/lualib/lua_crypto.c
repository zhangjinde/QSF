// Copyright (C) 2014-2015 chenqiang@chaoyuehudong.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <lua.h>
#include <lauxlib.h>
#include <openssl/evp.h>
#include <openssl/aes.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include "qsf.h"


#define RSA_BITS        1024
#define MAX_RSA_BUF     16384

#define AES_HANDLE      "qsfAES*"
#define RSA_HANDLE      "qsfRSA*"
#define HASH_HANDLE     "qsfHash*"

#define check_aes(L)   ((qsfAES*)luaL_checkudata(L, 1, AES_HANDLE))
#define check_rsa(L)   ((qsfRSA*)luaL_checkudata(L, 1, RSA_HANDLE))
#define check_hash(L)  ((qsfHash*)luaL_checkudata(L, 1, HASH_HANDLE))

typedef int(*RSAFuncType)(int, const uint8_t*, uint8_t*, RSA*, int);

typedef struct qsfAES
{
    uint8_t key[AES_BLOCK_SIZE];
    uint8_t iv[AES_BLOCK_SIZE];
}qsfAES;

typedef struct qsfRSA
{
    RSA     ctx;
    uint8_t buf[MAX_RSA_BUF];  //16KB
}qsfRSA;

typedef struct qsfHash
{
    EVP_MD_CTX ctx;
}qsfHash;

static void hex_dump(const void* data, int len, void* out, int outlen)
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
    if ( iv_len != AES_BLOCK_SIZE)
    {
        return luaL_error(L, "invalid AES init vector size: %d", key_len);
    }
    qsfAES* ptr = (qsfAES*)lua_newuserdata(L, sizeof(qsfAES));
    memcpy(ptr->key, key, key_len);
    memcpy(ptr->iv, iv, iv_len);
    luaL_getmetatable(L, AES_HANDLE);
    lua_setmetatable(L, -2);
    return 1;
}

static int aes_encrypt(lua_State* L, qsfAES* aes, const uint8_t* data, size_t size)
{
    uint8_t buffer[512];
    uint8_t* ptr = buffer;
    if (size > sizeof(buffer))
    {
        ptr = qsf_malloc(size);
    }
    int num = 0;
    AES_KEY ctx;
    uint8_t iv[AES_BLOCK_SIZE];
    memcpy(iv, aes->iv, sizeof(iv));
    AES_set_encrypt_key(aes->key, AES_BLOCK_SIZE * 8, &ctx);
    AES_ofb128_encrypt(data, ptr, size, &ctx, iv, &num);
    lua_pushlstring(L, ptr, size);
    if (ptr != buffer)
    {
        qsf_free(ptr);
    }
    return num;
}

static int crypto_aes_encrypt(lua_State* L)
{
    qsfAES* aes = check_aes(L);
    size_t size;
    const uint8_t* data = (const uint8_t*)luaL_checklstring(L, 2, &size);
    aes_encrypt(L, aes, data, size);
    return 1;
}

static int crypto_aes_decrypt(lua_State* L)
{
    qsfAES* aes = check_aes(L);
    size_t size;
    const uint8_t* data = (const uint8_t*)luaL_checklstring(L, 2, &size);
    aes_encrypt(L, aes, data, size);
    return 1;
}

//////////////////////////////////////////////////////////////////////////
static int crypto_rsa_new(lua_State* L)
{
    qsfRSA* rsa = (qsfRSA*)lua_newuserdata(L, sizeof(qsfRSA));
    memset(rsa, 0, sizeof(*rsa));
    luaL_getmetatable(L, RSA_HANDLE);
    lua_setmetatable(L, -2);
    return 1;
}

static int crypto_rsa_clear(lua_State* L)
{
    qsfRSA* rsa = check_rsa(L);
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
    qsfRSA* rsa = check_rsa(L);
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
    qsfRSA* rsa = check_rsa(L);
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

static int do_rsa_encrypt(qsfRSA* rsa, RSAFuncType func, const uint8_t* src, int size)
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

static int do_rsa_decrypt(qsfRSA* rsa, RSAFuncType func, const uint8_t* src, int size)
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
    qsfRSA* rsa = check_rsa(L);
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
    qsfRSA* rsa = check_rsa(L);
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
    qsfRSA* rsa = check_rsa(L);
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
    qsfRSA* rsa = check_rsa(L);
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

//////////////////////////////////////////////////////////////////////////
static int crypto_hash_new(lua_State* L)
{
    const char* type = luaL_checkstring(L, 1);
    const EVP_MD* md = NULL;
    if (strcmp(type, "md5") == 0)
    {
        md = EVP_md5();
    }
    else if (strcmp(type, "sha1") == 0)
    {
        md = EVP_sha1();
    }
    else if (strcmp(type, "sha256") == 0)
    {
        md = EVP_sha256();
    }
    else if (strcmp(type, "sha512") == 0)
    {
        md = EVP_sha512();
    }
    if (md == NULL)
    {
        return luaL_error(L, "invalid hash type");
    }
    qsfHash* ptr = (qsfHash*)lua_newuserdata(L, sizeof(qsfHash));
    EVP_MD_CTX_init(&ptr->ctx);
    EVP_DigestInit_ex(&ptr->ctx, md, NULL);
    luaL_getmetatable(L, HASH_HANDLE);
    lua_setmetatable(L, -2);
    return 1;
}

static int crypto_hash_gc(lua_State* L)
{
    qsfHash* hash = check_hash(L);
    EVP_MD_CTX_cleanup(&hash->ctx);
    return 0;
}

static int crypto_hash_update(lua_State* L)
{
    qsfHash* hash = check_hash(L);
    size_t length = 0;
    const char* data = luaL_checklstring(L, 2, &length);
    EVP_DigestUpdate(&hash->ctx, data, length);
    return 0;
}

static int crypto_hash_digest(lua_State* L)
{
    qsfHash* hash = check_hash(L);
    int ishex = 1;
    const char* option = luaL_optstring(L, 2, "hex");
    if (strcmp(option, "bin") == 0)
    {
        ishex = 0;
    }
    int length = 0;
    uint8_t data[EVP_MAX_MD_SIZE] = { '\0' };
    EVP_DigestFinal_ex(&hash->ctx, data, &length);
    if (ishex)
    {
        char hex[EVP_MAX_MD_SIZE * 2] = { '\0' };
        hex_dump(data, length, hex, sizeof(hex));
        lua_pushlstring(L, hex, length*2);
    }
    else
    {
        lua_pushlstring(L, data, length);
    }
    return 1;
}

//////////////////////////////////////////////////////////////////////////
static void create_meta(lua_State* L, const char* name, const luaL_Reg* methods)
{
    assert(L && name && methods);
    luaL_newmetatable(L, name);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, methods, 0);
    lua_pushliteral(L, "__metatable");
    lua_pushliteral(L, "cannot access this metatable");
    lua_settable(L, -3);
    lua_pop(L, 1);  /* pop new metatable */
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
        { "setPubKey", crypto_rsa_set_pubkey },
        { "setPriKey", crypto_rsa_set_key },
        { "pubEncrypt", crypto_rsa_pub_encrypt },
        { "pubDecrypt", crypto_rsa_pub_decrypt },
        { "priEncrypt", crypto_rsa_encrypt },
        { "priDecrypt", crypto_rsa_decrypt },
        { NULL, NULL },
    };
    static const luaL_Reg hash_lib[] = 
    {
        { "__gc", crypto_hash_gc },
        { "clear", crypto_hash_gc },
        { "update", crypto_hash_update },
        { "digest", crypto_hash_digest },
        { NULL, NULL },
    };

    create_meta(L, AES_HANDLE, aes_lib);
    create_meta(L, RSA_HANDLE, rsa_lib);
    create_meta(L, HASH_HANDLE, hash_lib);
}

LUALIB_API int luaopen_crypto(lua_State* L)
{
    static const luaL_Reg lib[] =
    {
        { "createAES", crypto_aes_new },
        { "createRSA", crypto_rsa_new }, 
        { "createHash", crypto_hash_new },
        { "genRSAKeypair", crypto_rsa_gen_keypair },
        { NULL, NULL },
    };

    luaL_newlib(L, lib);
    make_meta(L);
    return 1;
}
