/*
** Copyright (c) 2014-2017 uboss.org All rights Reserved.
** uBoss - A Lightweight MicroService Framework
**
** uBoss Daemon
**
** Dali Wang<dali@uboss.org>
** See Copyright Notice in uboss.h
*/

#ifndef UBOSS_DAEMON_H
#define UBOSS_DAEMON_H

int daemon_init(const char *pidfile);
int daemon_exit(const char *pidfile);

#endif
