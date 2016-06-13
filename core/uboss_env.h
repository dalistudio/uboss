/*
** Copyright (c) 2014-2016 uboss.org All rights Reserved.
** uBoss - A lightweight micro service framework
**
** uBoss Lua Env
**
** Dali Wang<dali@uboss.org>
** See Copyright Notice in uboss.h
*/

/*
 * Lua 全局环境
 *
 * 用一个 luaVM 管理所有全局变量
*/
#ifndef UBOSS_ENV_H
#define UBOSS_ENV_H

const char * uboss_getenv(const char *key);
void uboss_setenv(const char *key, const char *value);

void uboss_env_init();

#endif
