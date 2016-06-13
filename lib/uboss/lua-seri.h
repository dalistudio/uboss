/*
** Copyright (c) 2014-2016 uboss.org All rights Reserved.
** uBoss - A lightweight micro service framework
**
** uBoss Lua Binding uboss Library
**
** Dali Wang<dali@uboss.org>
** See Copyright Notice in uboss.h
*/

#ifndef LUA_SERIALIZE_H
#define LUA_SERIALIZE_H

#include <lua.h>

int luaseri_pack(lua_State *L);
int luaseri_unpack(lua_State *L);

#endif
