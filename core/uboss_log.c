/*
** Copyright (c) 2014-2017 uboss.org All rights Reserved.
** uBoss - A Lightweight MicroService Framework
**
** uBoss Log
**
** Dali Wang<dali@uboss.org>
** See Copyright Notice in uboss.h
*/

#include "uboss_log.h"
#include "uboss_timer.h"
#include "uboss.h"
#include "uboss_socket.h"
#include <string.h>
#include <time.h>

// 打开日志
FILE * 
uboss_log_open(struct uboss_context * ctx, uint32_t handle) {
	const char * logpath = uboss_getenv("logpath"); // 获得日志的路径
	if (logpath == NULL)
		return NULL;
	size_t sz = strlen(logpath); // 获得日志路径的长度
	char tmp[sz + 16];
	sprintf(tmp, "%s/%08x.log", logpath, handle); // 生成日志文件路径和名字
	FILE *f = fopen(tmp, "ab"); // 打开文件
	if (f) {
		uint32_t starttime = uboss_starttime(); // uBoss 启动时间
		uint64_t currenttime = uboss_now(); // 获得现在时间
		time_t ti = starttime + currenttime/100;
		uboss_error(ctx, "Open log file %s", tmp); // 输出日志到框架
		fprintf(f, "open time: %u %s", (uint32_t)currenttime, ctime(&ti));
		fflush(f);
	} else {
		uboss_error(ctx, "Open log file %s fail", tmp);
	}
	return f;
}

// 关闭日志
void
uboss_log_close(struct uboss_context * ctx, FILE *f, uint32_t handle) {
	uboss_error(ctx, "Close log file :%08x", handle);
	fprintf(f, "close time: %u\n", (uint32_t)uboss_now());
	fclose(f); // 关闭文件流
}

// 以二进制块方式，写入日志
static void
log_blob(FILE *f, void * buffer, size_t sz) {
	size_t i;
	uint8_t * buf = buffer;
	for (i=0;i!=sz;i++) {
		fprintf(f, "%02x", buf[i]); // 将日志写入 文件流
	}
}

// 来自消息队列中的消息数据，写入日志
static void
log_socket(FILE * f, struct uboss_socket_message * message, size_t sz) {
	fprintf(f, "[socket] %d %d %d ", message->type, message->id, message->ud);

	if (message->buffer == NULL) {
		const char *buffer = (const char *)(message + 1);
		sz -= sizeof(*message); // 计算消息的长度
		const char * eol = memchr(buffer, '\0', sz); // 查找 '\0' 字符出现的位置
		if (eol) {
			sz = eol - buffer;
		}
		fprintf(f, "[%*s]", (int)sz, (const char *)buffer); // 将日志写入 文件流
	} else {
		sz = message->ud;
		log_blob(f, message->buffer, sz); // 将消息的数据，以二进制块写入日志
	}
	fprintf(f, "\n");
	fflush(f); // 清空缓冲区，并输出数据到流
}

// 输出日志
void 
uboss_log_output(FILE *f, uint32_t source, int type, int session, void * buffer, size_t sz) {
	if (type == PTYPE_SOCKET) {
		log_socket(f, buffer, sz); // 来自消息的数据
	} else {
		uint32_t ti = (uint32_t)uboss_now();
		fprintf(f, ":%08x %d %d %u ", source, type, session, ti);
		log_blob(f, buffer, sz); // 写入日志块
		fprintf(f,"\n");
		fflush(f); // 清空缓冲区，并输出数据到流
	}
}
