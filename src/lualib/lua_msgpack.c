// Copyright (C) 2015 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#include <assert.h>
#include <stdint.h>
#include <float.h>
#include <lua.h>
#include <lauxlib.h>
#include <msgpack.h>

#define SHORT_STR_SIZE    256

// pack Lua value to msgpack format
static void mp_pack_value(lua_State* L, msgpack_packer* packer);
static void mp_pack_array(lua_State* L, msgpack_packer* packer);
static void mp_pack_map(lua_State* L, msgpack_packer* packer);

// unpack msgpack data to Lua value
static void mp_unpack_value(lua_State* L, const msgpack_object* value);
static void mp_unpack_array(lua_State* L, const msgpack_object* value);
static void mp_unpack_map(lua_State* L, const msgpack_object* value);

// is this table an array or hash
static bool table_is_array(lua_State* L)
{
    assert(lua_type(L, -1) == LUA_TTABLE);
    int len = 0;
    lua_pushnil(L);
    while (lua_next(L, -2) != 0)
    {
        if (lua_isinteger(L, -2))
        {
            if (lua_tointeger(L, -2) == ++len)
            {
                lua_pop(L, 1);
                continue;
            }
        }
        lua_pop(L, 2);
        return false;
    }
    return true;
}

static size_t table_size(lua_State* L)
{
    assert(lua_type(L, -1) == LUA_TTABLE);
    size_t len = 0;
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
        if (val < FLT_MAX && val > FLT_MIN)
            msgpack_pack_float(packer, (float)val);
        else
            msgpack_pack_double(packer, val);
    }
}

// pack Lua array table
static void mp_pack_array(lua_State* L, msgpack_packer* packer)
{
    assert(L && packer && lua_type(L, -1) == LUA_TTABLE);
    size_t len = lua_rawlen(L, -1);
    msgpack_pack_array(packer, len);
    for (size_t i = 1; i <= len; i++)
    {
        lua_rawgeti(L, -1, i);
        mp_pack_value(L, packer);
        lua_pop(L, 1);
    }
}

// pack Lua table, with hash part
static void mp_pack_map(lua_State* L, msgpack_packer* packer)
{
    assert(L && packer && lua_type(L, -1) == LUA_TTABLE);
    msgpack_pack_map(packer, table_size(L));
    lua_pushnil(L);
    while (lua_next(L, -2) != 0)
    {
        size_t len = 0;
        const char* key = luaL_checklstring(L, -2, &len);
        msgpack_pack_str(packer, len);
        msgpack_pack_str_body(packer, key, len);
        mp_pack_value(L, packer);
        lua_pop(L, 1);
    }
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
            msgpack_pack_str(packer, len);
            msgpack_pack_str_body(packer, str, len);
        }
        break;

    case LUA_TNUMBER:
        {
            mp_pack_number(L, packer);
        }
        break;

    case LUA_TTABLE:
        {
            if (table_is_array(L))
                mp_pack_array(L, packer);
            else
                mp_pack_map(L, packer);
        }
        break;

    default:
        // `msgpack_sbuffer` is not freed, thus memory leak occurs.
        luaL_error(L, "cant encode value of type: %s", lua_typename(L, type)); 
    }
}

static int mp_pack(lua_State* L)
{
    msgpack_sbuffer* buf = msgpack_sbuffer_new();
    msgpack_packer packer;
    msgpack_packer_init(&packer, buf, msgpack_sbuffer_write);

    // not exception-safe
    mp_pack_value(L, &packer);
    lua_pushlstring(L, buf->data, buf->size);

    msgpack_sbuffer_free(buf);
    return 1;
}

// unpack msgpack array to Lua table
static void mp_unpack_array(lua_State* L, const msgpack_object* value)
{
    assert(L && value && value->type == MSGPACK_OBJECT_ARRAY);
    const msgpack_object_array* array = &value->via.array;
    lua_createtable(L, array->size, 0);
    int index = 1;
    for (uint32_t i = 0; i < array->size; i++)
    {
        mp_unpack_value(L, &array->ptr[i]);
        lua_rawseti(L, -2, index++);
    }
}

// `k` for lua_setfield must be a null-terminated string
inline void mp_set_map_key(lua_State* L, const msgpack_object_str* key)
{
    char short_str[SHORT_STR_SIZE];
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
static void mp_unpack_map(lua_State* L, const msgpack_object* value)
{
    assert(L && value && value->type == MSGPACK_OBJECT_MAP);
    const msgpack_object_map* map = &value->via.map;
    lua_createtable(L, 0, map->size);
    for (uint32_t i = 0; i < map->size; i++)
    {
        const msgpack_object_kv* item = &map->ptr[i];
        const msgpack_object* key = &item->key;
        assert(key->type == MSGPACK_OBJECT_STR);
        mp_unpack_value(L, &item->val);
        mp_set_map_key(L, &key->via.str);
    }
}

static void mp_unpack_value(lua_State* L, const msgpack_object* value)
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
        mp_unpack_array(L, value);;
        break;

    case MSGPACK_OBJECT_MAP:
        mp_unpack_map(L, value);;
        break;

    case MSGPACK_OBJECT_EXT:
        // `msgpack_unpacked` object is not freed, thus memory leak occurs.
        luaL_error(L, "unpack type of `ext` is not implemented");
    }
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
        // not exception-safe
        mp_unpack_value(L, &msg.data);

        msgpack_unpacked_destroy(&msg);
        return 1;
    }
    else
    {
        msgpack_unpacked_destroy(&msg);
        return luaL_error(L, "unpack failed at offset: %d", offset);
    }
}

LUALIB_API int luaopen_msgpack(lua_State* L)
{
    static const luaL_Reg lib[] =
    {
        { "pack", mp_pack },
        { "unpack", mp_unpack },
        { NULL, NULL },
    };
    luaL_newlib(L, lib);
    return 1;
}