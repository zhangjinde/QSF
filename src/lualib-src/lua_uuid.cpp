#include <assert.h>
#include <stdint.h>
#include <lua.hpp>
#include "core/logging.h"

#ifdef _WIN32
#include <Objbase.h>
#else
#include <uuid/uuid.h>
#endif

inline void uuid_create(void* buf)
{
    assert(buf);
#ifdef _WIN32
    CoCreateGuid((GUID*)buf);
#else
    uuid_generate((uint8_t*)buf);
#endif
}

inline int uuid_compare(const void* uuid1, const void* uuid2)
{
    assert(uuid1 && uuid2);
#ifdef _WIN32
    return memcmp(uuid1, uuid2, sizeof(GUID));
#else
    return uuid_compare((uint8_t*)uuid1, (uint8_t*)uuid2);
#endif
}

inline void uuid_tostring(const void* uuid, char* out, int len)
{
#ifdef _WIN32
    const GUID* guid = (const GUID*)uuid;
    const char* fmt = "{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X}";
    sprintf_s(out, len, fmt, guid->Data1, guid->Data2, guid->Data3,
        guid->Data4[0], guid->Data4[1], guid->Data4[2], guid->Data4[3],
        guid->Data4[4], guid->Data4[5], guid->Data4[6], guid->Data4[7]);
#else
    uuid_unparse((uint8_t*)uuid, out);
#endif
}

#define UUID_HANDLE         "UUID*"
#define check_uuid(L, idx)  (luaL_checkudata(L, idx, UUID_HANDLE))

static int new_uuid(lua_State* L)
{
    void* udata = lua_newuserdata(L, 16);
    uuid_create(udata);
    luaL_getmetatable(L, UUID_HANDLE);
    lua_setmetatable(L, -2);
    return 1;
}

static int uuid_gc(lua_State* L)
{
    void* uuid = check_uuid(L, 1);
    memset(uuid, 0, 16);
    return 0;
}

static int uuid_tostring(lua_State* L)
{
    void* uuid = check_uuid(L, 1);
    char buffer[40] = {};
    uuid_tostring(uuid, buffer, 40);
    lua_pushstring(L, buffer);
    return 1;
}

static int uuid_len(lua_State* L)
{
    lua_pushinteger(L, 16);
    return 1;
}

static int uuid_equal(lua_State* L)
{
    void* uuid1 = check_uuid(L, 1);
    void* uuid2 = check_uuid(L, 2);
    int r = uuid_compare(uuid1, uuid2);
    lua_pushboolean(L, r == 0);
    return 1;
}

static int uuid_less_than(lua_State* L)
{
    void* uuid1 = check_uuid(L, 1);
    void* uuid2 = check_uuid(L, 2);
    int r = uuid_compare(uuid1, uuid2);
    lua_pushboolean(L, r < 0);
    return 1;
}

static int uuid_less_equal(lua_State* L)
{
    void* uuid1 = check_uuid(L, 1);
    void* uuid2 = check_uuid(L, 2);
    int r = uuid_compare(uuid1, uuid2);
    lua_pushboolean(L, r <= 0);
    return 1;
}

static void make_uuid_meta(lua_State* L)
{
    static const luaL_Reg metalib[] =
    {
        { "__gc", uuid_gc },
        { "__tostring", uuid_tostring },
        { "__len", uuid_len },
        { "__eq", uuid_equal },
        { "__lt", uuid_less_than },
        { "__le", uuid_less_equal },
        { NULL, NULL },
    };
    luaL_newmetatable(L, UUID_HANDLE);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, metalib, 0);
    lua_pop(L, 1);  /* pop new metatable */
}

extern "C"
int luaopen_uuid(lua_State* L)
{
    static const luaL_Reg lib[] =
    {
        { "create", new_uuid },
        { NULL, NULL },
    };
    luaL_newlib(L, lib);
    make_uuid_meta(L);
    return 1;
}
