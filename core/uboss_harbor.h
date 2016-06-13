/*
** Copyright (c) 2014-2016 uboss.org All rights Reserved.
** uBoss - A lightweight micro service framework
**
** uBoss Harbor
**
** Dali Wang<dali@uboss.org>
** See Copyright Notice in uboss.h
*/

/*
 * 集群模块
 *
 * 主要判断发送的消息是否为远程，以及发送远程消息
*/
#ifndef UBOSS_HARBOR_H
#define UBOSS_HARBOR_H

#include <stdint.h>
#include <stdlib.h>

#define GLOBALNAME_LENGTH 16
#define REMOTE_MAX 256

struct remote_name {
	char name[GLOBALNAME_LENGTH];
	uint32_t handle;
};

struct remote_message {
	struct remote_name destination;
	const void * message;
	size_t sz;
};

void uboss_harbor_send(struct remote_message *rmsg, uint32_t source, int session);
int uboss_harbor_message_isremote(uint32_t handle);
void uboss_harbor_init(int harbor);
void uboss_harbor_start(void * ctx);
void uboss_harbor_exit();

#endif
