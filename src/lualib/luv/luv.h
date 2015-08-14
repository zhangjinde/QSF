// Copyright (C) 2014-2015 chenqiang@chaoyuehudong.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#pragma once


// loop -> state
#define luv_state(loop) ((lua_State*)loop->data)

// state -> loop
static uv_loop_t* luv_loop(lua_State* L);

// string representation of address family
static const char* luv_af_num_to_string(int num);

// uv error to lua exception
static int luv_error(lua_State* L, int r);
