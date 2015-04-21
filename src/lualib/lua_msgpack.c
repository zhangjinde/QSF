// Copyright (C) 2015 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#include <assert.h>
#include <stdint.h>
#include <math.h>
#include <float.h>
#include <lua.h>
#include <lauxlib.h>

#define MSGPACK_SBUFFER_INIT_SIZE   1024
#include <msgpack.h>

#define MP_SHORT_STR_SIZE   256
#define MP_MAX(a, b)        ((a) > (b) ? (a) : (b))

#define MP_PACK_FINAL(pk)   msgpack_sbuffer_destroy((msgpack_sbuffer*)(pk)->data)

#define MP_VALIDATE_NUMBER(pk, v) \
    if (!mp_config.no_validate_number && (isinf(v) || isnan(v))) \
    { \
        MP_PACK_FINAL(pk);\
        luaL_error(L, "number is NaN or Infinity");\
    }

typedef struct mp_config_s
{
    int sparse_array_as_map;
    int no_validate_number;
    int encode_str_bin;
}mp_config_t;

static mp_config_t mp_config;

// pack Lua value to msgpack format
static void mp_pack_value(lua_State* L, msgpack_packer* packer);
static void mp_pack_array(lua_State* L, msgpack_packer* packer, int size);
static void mp_pack_map(lua_State* L, msgpack_packer* packer);

// unpack msgpack data to Lua value
static int mp_unpack_value(lua_State* L, const msgpack_object* value);
static int mp_unpack_array(lua_State* L, const msgpack_object* value);
static int mp_unpack_map(lua_State* L, const msgpack_object* value);

static int mp_array_size(lua_State* L)
{
    assert(lua_type(L, -1) == LUA_TTABLE);
    int k = 0, max = 0, items = 0;
    lua_pushnil(L);
    while (lua_next(L, -2) != 0)
    {
        if (lua_isinteger(L, -2))
        {
            k = (int)lua_tointeger(L, -2);
            if (k >= 1)
            {
                max = MP_MAX(k, max);
                items++;
                lua_pop(L, 1);
                continue;
            }
        }
        /* Must not be an array (non integer key) */
        lua_pop(L, 2);
        return -1;
    }
    if (mp_config.sparse_array_as_map && max > items)
    {
        return -1;
    }
    return max;
}

static int mp_table_size(lua_State* L)
{
    assert(lua_type(L, -1) == LUA_TTABLE);
    int len = 0;
    lua_pushnil(L);
    while (lua_next(L, -2) != 0)
    {
        ++len;
        lua_pop(L, 1);
    }
    return len;
}

// pack Lua number value 
static void mp_pack_number(lua_State* L, msgpack_packer* packer)
{
    if (lua_isinteger(L, -1))
    {
        int64_t val = luaL_checkinteger(L, -1);
        msgpack_pack_int64(packer, val);
    }
    else
    {
        double val = luaL_checknumber(L, -1);
        MP_VALIDATE_NUMBER(packer, val);
        if (val > FLT_MIN && val < FLT_MAX)
            msgpack_pack_float(packer, (float)val);
        else
            msgpack_pack_double(packer, val);
    }
}

// pack Lua array table
static void mp_pack_array(lua_State* L, msgpack_packer* packer, int size)
{
    assert(L && packer && lua_type(L, -1) == LUA_TTABLE);
    msgpack_pack_array(packer, size);
    for (size_t i = 1; i <= size; i++)
    {
        lua_rawgeti(L, -1, i);
        mp_pack_value(L, packer);
        lua_pop(L, 1);
    }
}

// unpack msgpack array to Lua table
static int mp_unpack_array(lua_State* L, const msgpack_object* value)
{
    assert(L && value && value->type == MSGPACK_OBJECT_ARRAY);
    const msgpack_object_array* array = &value->via.array;
    lua_createtable(L, array->size, 0);
    int index = 1;
    for (uint32_t i = 0; i < array->size; i++)
    {
        if (mp_unpack_value(L, &array->ptr[i]) == 0)
        {
            lua_rawseti(L, -2, index++);
        }
        else
        {
            return -1;
        }
    }
    return 0;
}

// pack Lua table, with hash part
static void mp_pack_map(lua_State* L, msgpack_packer* packer)
{
    assert(L && packer && lua_type(L, -1) == LUA_TTABLE);
    msgpack_pack_map(packer, mp_table_size(L));
    lua_pushnil(L);
    while (lua_next(L, -2) != 0)
    {
        int t = lua_type(L, -2);
        switch (t)
        {
        case LUA_TNUMBER:
            mp_pack_number(L, packer);
            break;
        case LUA_TSTRING:
            {
                size_t len = 0;
                const char* key = luaL_checklstring(L, -2, &len);
                msgpack_pack_str(packer, len);
                msgpack_pack_str_body(packer, key, len);
            }
            break;
        default:
            MP_PACK_FINAL(packer);
            luaL_error(L, "invalid table key type: %s", lua_typename(L, t));
            break;
        }
        mp_pack_value(L, packer);
        lua_pop(L, 1);
    }
}

// `k` for lua_setfield must be a null-terminated string
inline void mp_set_map_key(lua_State* L, const msgpack_object_str* key)
{
    char short_str[MP_SHORT_STR_SIZE];
    if (key->size < sizeof(short_str))
    {
        strncpy(short_str, key->ptr, key->size);
        short_str[key->size] = '\0';
        lua_setfield(L, -2, short_str);
    }
    else
    {
        char* buf = malloc(key->size + 1);
        strncpy(buf, key->ptr, key->size);
        buf[key->size] = '\0';
        lua_setfield(L, -2, buf);
        free(buf);
    }
}

// unpack msgpack map(dictionary) to Lua table
static int mp_unpack_map(lua_State* L, const msgpack_object* value)
{
    assert(L && value && value->type == MSGPACK_OBJECT_MAP);
    const msgpack_object_map* map = &value->via.map;
    lua_createtable(L, 0, map->size);
    for (uint32_t i = 0; i < map->size; i++)
    {
        const msgpack_object_kv* item = &map->ptr[i];
        const msgpack_object* key = &item->key;
        assert(key->type == MSGPACK_OBJECT_STR);
        if (mp_unpack_value(L, &item->val) == 0)
        {
            mp_set_map_key(L, &key->via.str);
        }
        else
        {
            return -1;
        }
    }
    return 0;
}

// pack Lua value to msgpack format string
static void mp_pack_value(lua_State* L, msgpack_packer* packer)
{
    assert(L && packer);
    int type = lua_type(L, -1);
    switch (type)
    {
    case LUA_TNIL:
        msgpack_pack_nil(packer);
        break;

    case LUA_TBOOLEAN:
        {
            int b = lua_toboolean(L, -1);
            if (b == 0)
                msgpack_pack_false(packer);
            else
                msgpack_pack_true(packer);
        }
        break;

    case LUA_TSTRING:
        {
            size_t len = 0;
            const char* str = luaL_checklstring(L, -1, &len);
            if (mp_config.encode_str_bin)
            {
                msgpack_pack_bin(packer, len);
                msgpack_pack_bin_body(packer, str, len);
            }
            else
            {
                msgpack_pack_str(packer, len);
                msgpack_pack_str_body(packer, str, len);
            }
        }
        break;

    case LUA_TNUMBER:
        mp_pack_number(L, packer);
        break;

    case LUA_TTABLE:
        {
            int size = mp_array_size(L);
            if (size >= 0)
            {
                mp_pack_array(L, packer, size);
            }
            else
            {
                mp_pack_map(L, packer);
            }
        }
        break;

    default:
        MP_PACK_FINAL(packer);
        luaL_error(L, "cant encode value of type: %s", lua_typename(L, type)); 
    }
}

static int mp_unpack_value(lua_State* L, const msgpack_object* value)
{
    assert(L && value);
    switch (value->type)
    {
    case MSGPACK_OBJECT_NIL:
        lua_pushnil(L);
        break;

    case MSGPACK_OBJECT_BOOLEAN:
        lua_pushboolean(L, value->via.boolean);
        break;

    case MSGPACK_OBJECT_POSITIVE_INTEGER:
        {
            uint64_t val = value->via.u64;
            if (val <= INT64_MAX)
                lua_pushinteger(L, val);
            else
                lua_pushnumber(L, (lua_Number)val);
        }
        break;

    case MSGPACK_OBJECT_NEGATIVE_INTEGER:
        {
            int64_t val = value->via.i64;
            if (val >= INT64_MIN)
                lua_pushinteger(L, val);
            else
                lua_pushnumber(L, (lua_Number)val);
        }
        break;

    case MSGPACK_OBJECT_FLOAT:
        lua_pushnumber(L, value->via.f64);
        break;

    case MSGPACK_OBJECT_STR:
        {
            const msgpack_object_str* str = &value->via.str;
            lua_pushlstring(L, str->ptr, str->size);
        }
        break;

    case MSGPACK_OBJECT_BIN:
        {
            const msgpack_object_bin* bin = &value->via.bin;
            lua_pushlstring(L, bin->ptr, bin->size);
        }
        break;

    case MSGPACK_OBJECT_ARRAY:
        return mp_unpack_array(L, value);

    case MSGPACK_OBJECT_MAP:
        return mp_unpack_map(L, value);

    default:
        return -1;
    }
    return 0;
}

static int mp_pack(lua_State* L)
{
    msgpack_sbuffer buf;
    msgpack_sbuffer_init(&buf);
    msgpack_packer packer;
    msgpack_packer_init(&packer, &buf, msgpack_sbuffer_write);

    // not exception-safe
    mp_pack_value(L, &packer);
    lua_pushlstring(L, buf.data, buf.size);

    msgpack_sbuffer_destroy(&buf);
    return 1;
}

static int mp_unpack(lua_State* L)
{
    size_t len = 0;
    size_t offset = 0;
    const char* data = luaL_checklstring(L, -1, &len);
    msgpack_unpacked msg;
    msgpack_unpacked_init(&msg);
    int err = msgpack_unpack_next(&msg, data, len, &offset);
    if (err == MSGPACK_UNPACK_SUCCESS)
    {
        int r = mp_unpack_value(L, &msg.data);
        msgpack_unpacked_destroy(&msg);
        return (r == 0 ? 1 : 0);
    }
    else
    {
        msgpack_unpacked_destroy(&msg);
        return luaL_error(L, "unpack failed at offset: %d", offset);
    }
}

static int mp_set_option(lua_State* L)
{
    const char* option = luaL_checkstring(L, 1);
    int old_value = 0;
    int enable = lua_toboolean(L, 2);
    if (strcmp(option, "encode_array_as_map") == 0)
    {
        old_value = mp_config.sparse_array_as_map;
        mp_config.sparse_array_as_map = enable;
    }
    else if (strcmp(option, "validate_number") == 0)
    {
        old_value = !mp_config.no_validate_number;
        mp_config.no_validate_number = !enable;
    }
    else if (strcmp(option, "encode_str_bin") == 0)
    {
        old_value = mp_config.encode_str_bin;
        mp_config.encode_str_bin = enable;
    }
    lua_pushinteger(L, old_value);
    return 1;
}

LUALIB_API int luaopen_msgpack(lua_State* L)
{
    static const luaL_Reg lib[] =
    {
        { "pack", mp_pack },
        { "unpack", mp_unpack },
        { "set_option", mp_set_option },
        { NULL, NULL },
    };

    luaL_newlib(L, lib);
    return 1;
}