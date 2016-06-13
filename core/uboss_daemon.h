/*
** Copyright (c) 2014-2016 uboss.org All rights Reserved.
** uBoss - A lightweight micro service framework
**
** uBoss Daemon
**
** Dali Wang<dali@uboss.org>
** See Copyright Notice in uboss.h
*/

/*
 * 守护模块
 *
 * 
*/
#ifndef UBOSS_DAEMON_H
#define UBOSS_DAEMON_H

int daemon_init(const char *pidfile);
int daemon_exit(const char *pidfile);

#endif
