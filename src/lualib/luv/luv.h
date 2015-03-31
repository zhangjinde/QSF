// Copyright (C) 2014-2015 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#pragma once


#define DEFAULT_BUF_SIZE    8196

#define push_literal(L, name, value)    \
    lua_pushstring(L, name);            \
    lua_pushinteger(L, value);          \
    lua_rawset(L, -3);

#define luv_error(L, r) luaL_error(L, "%s: %s", uv_err_name((r)), uv_strerror((r)));


uv_loop_t* luv_loop(lua_State* L);
int luv_check_ref(lua_State* L, int idx);
int luv_new_tcp(lua_State* L);
int luv_new_timer(lua_State* L);

int luv_tcp_init(lua_State* L);
int luv_timer_init(lua_State* L);
