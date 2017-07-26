/*
** Copyright (c) 2014-2017 uboss.org All rights Reserved.
** uBoss - A Lightweight MicroService Framework
**
** uBoss Error
**
** Dali Wang<dali@uboss.org>
** See Copyright Notice in uboss.h
*/

#include "uboss.h"
#include "uboss_handle.h"
#include "uboss_mq.h"
#include "uboss_server.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define LOG_MESSAGE_SIZE 256

// 将 uBoss 框架的日志，写入 logger 日志记录器
void 
uboss_error(struct uboss_context * context, const char *msg, ...) {
	static uint32_t logger = 0;
	if (logger == 0) {
		logger = uboss_handle_findname("logger"); // 查找 logger 日志记录器服务的 句柄值
	}
	if (logger == 0) {
		return; // 没有找到 句柄值 则返回
	}

	char tmp[LOG_MESSAGE_SIZE]; // 临时日志消息数组
	char *data = NULL;

	va_list ap;

	va_start(ap,msg);
	int len = vsnprintf(tmp, LOG_MESSAGE_SIZE, msg, ap); // 打印可变参数到临时数组
	va_end(ap);
	if (len >=0 && len < LOG_MESSAGE_SIZE) {
		data = uboss_strdup(tmp);
	} else {
		int max_size = LOG_MESSAGE_SIZE;
		for (;;) {
			max_size *= 2;
			data = uboss_malloc(max_size);
			va_start(ap,msg);
			len = vsnprintf(data, max_size, msg, ap);
			va_end(ap);
			if (len < max_size) {
				break;
			}
			uboss_free(data);
		}
	}
	if (len < 0) {
		uboss_free(data);
		perror("vsnprintf error :");
		return;
	}


	struct uboss_message smsg;
	if (context == NULL) {
		smsg.source = 0;
	} else {
		smsg.source = uboss_context_handle(context);
	}
	smsg.session = 0;
	smsg.data = data;
	smsg.sz = len | ((size_t)PTYPE_TEXT << MESSAGE_TYPE_SHIFT);
	uboss_context_push(logger, &smsg); // 将消息压入 logger 日志记录器
}

