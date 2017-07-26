/*
** Copyright (c) 2014-2017 uboss.org All rights Reserved.
** uBoss - A Lightweight MicroService Framework
**
** uBoss Harbor
**
** Dali Wang<dali@uboss.org>
** See Copyright Notice in uboss.h
*/

#include "uboss.h"
#include "uboss_harbor.h"
#include "uboss_server.h"
#include "uboss_mq.h"
#include "uboss_handle.h"

#include <string.h>
#include <stdio.h>
#include <assert.h>

static struct uboss_context * REMOTE = 0;
static unsigned int HARBOR = ~0;

// 发送消息到集群
void 
uboss_harbor_send(struct remote_message *rmsg, uint32_t source, int session) {
	int type = rmsg->sz >> MESSAGE_TYPE_SHIFT;
	rmsg->sz &= MESSAGE_TYPE_MASK;
	assert(type != PTYPE_SYSTEM && type != PTYPE_HARBOR && REMOTE);

	// 远程消息将发往 REMOTE 指向的服务处理，这个服务为 module_harbor。
	uboss_context_send(REMOTE, rmsg, sizeof(*rmsg) , source, type , session);
}

// 判断是否为远程消息
int 
uboss_harbor_message_isremote(uint32_t handle) {
	assert(HARBOR != ~0);
	int h = (handle & ~HANDLE_MASK);
	return h != HARBOR && h !=0;
}

// 初始化集群
void
uboss_harbor_init(int harbor) {
	HARBOR = (unsigned int)harbor << HANDLE_REMOTE_SHIFT; // 将集群的值 移位 到高位
}

// 启动集群
void
uboss_harbor_start(void *ctx) {
	//
	// 这个函数将在 module_harbor 中调用，并将它的 context 赋给函数。
	// 这样框架中的 harbor 使用的 ctx 就是 harbor模块来处理。
	//

	// 这个 HARBOR 必须是预留的，以确保指针是有效的
	// the HARBOR must be reserved to ensure the pointer is valid.
	// 它最终将在 uboss_harbor_exit 中释放。
	// It will be released at last by calling uboss_harbor_exit
	uboss_context_reserve(ctx); // 保留
	REMOTE = ctx;
}

// 退出集群
void
uboss_harbor_exit() {
	struct uboss_context * ctx = REMOTE;
	REMOTE= NULL;
	if (ctx) {
		uboss_context_release(ctx);
	}
}
