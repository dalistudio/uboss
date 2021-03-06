/*
** Copyright (c) 2014-2016 uboss.org All rights Reserved.
** uBoss - A Lightweight MicroService Framework
**
** uBoss Lua Shared Table
**
** Dali Wang<dali@uboss.org>
** See Copyright Notice in uboss.h
*/

#ifndef LUA_SHORT_STRING_TABLE_H
#define LUA_SHORT_STRING_TABLE_H

#include "lstring.h"

// 如果你使用修改的 lua， 则 宏定义在 lua 源代码的 lstring.h 中
#ifndef ENABLE_SHORT_STRING_TABLE

static inline int luaS_shrinfo(lua_State *L) { return 0; }
static inline void luaS_initshr() {}
static inline void luaS_exitshr() {}
static inline void luaS_expandshr(int n) {}

#endif

#endif
