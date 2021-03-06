/*
** Copyright (c) 2014-2016 uboss.org All rights Reserved.
** uBoss - A Lightweight MicroService Framework
**
** uBoss Server
**
** Dali Wang<dali@uboss.org>
** See Copyright Notice in uboss.h
*/

#include "uboss.h"

#include "uboss_server.h"
#include "uboss_module.h"
#include "uboss_handle.h"
#include "uboss_monitor.h"
#include "uboss_mq.h"
#include "uboss_log.h"
#include "uboss_harbor.h"
#include "uboss_start.h"
#include "uboss_lock.h"
#include "uboss_timer.h"
#include "atomic.h"

#include <pthread.h>

#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>


// 节点总数
int 
uboss_context_total() {
	return G_NODE.total;
}

// 节点加一
static void
context_inc() {
	ATOM_INC(&G_NODE.total);
}

// 节点减一
static void
context_dec() {
	ATOM_DEC(&G_NODE.total);
}

// 获得当前上下文的句柄值
uint32_t 
uboss_current_handle(void) {
	if (G_NODE.init) {
		void * handle = pthread_getspecific(G_NODE.handle_key);
		return (uint32_t)(uintptr_t)handle;
	} else {
		uint32_t v = (uint32_t)(-THREAD_MAIN);
		return v;
	}
}

//
struct drop_t {
	uint32_t handle;
};

// 退出消息
static void
drop_message(struct uboss_message *msg, void *ud) {
	struct drop_t *d = ud;
	uboss_free(msg->data); // 释放消息的数据内存空间
	uint32_t source = d->handle;
	assert(source);
	// report error to the message source
	uboss_send(NULL, source, msg->source, PTYPE_ERROR, 0, NULL, 0); // 发送消息的来源地址
}

// 新建 uBoss 的上下文
struct uboss_context * 
uboss_context_new(const char * name, const char *param) {
	struct uboss_module * mod = uboss_module_query(name); // 根据模块名，查找模块数组中模块的指针

	if (mod == NULL) // 如果指针为空
		return NULL; // 直接返回

	// 找到模块，则取出 创建函数 的指针
	void *inst = uboss_module_instance_create(mod);
	if (inst == NULL) // 如果指针为空
		return NULL; // 直接返回

	// 分配 uBoss 结构的内存空间
	struct uboss_context * ctx = uboss_malloc(sizeof(*ctx));
	CHECKCALLING_INIT(ctx)

	ctx->mod = mod;
	ctx->instance = inst;
	ctx->ref = 2;
	ctx->cb = NULL;
	ctx->cb_ud = NULL;
	ctx->session_id = 0;
	ctx->logfile = NULL;

	ctx->init = false;
	ctx->endless = false;
	// Should set to 0 first to avoid uboss_handle_retireall get an uninitialized handle
	ctx->handle = 0; // 先赋值为0，分配内存空间
	ctx->handle = uboss_handle_register(ctx); // 再注册到句柄注册中心，获得句柄值
	struct message_queue * queue = ctx->queue = uboss_mq_create(ctx->handle); // 创建消息队列
	// init function maybe use ctx->handle, so it must init at last
	context_inc(); // 上下文 加一

	CHECKCALLING_BEGIN(ctx)
	int r = uboss_module_instance_init(mod, inst, ctx, param); // 从模块指针中获取 初始化函数的指针
	CHECKCALLING_END(ctx)
	if (r == 0) { // 初始化函数返回值为0，表示正常。
		struct uboss_context * ret = uboss_context_release(ctx); // 检查上下文释放标志 ref 是否为0
		if (ret) {
			ctx->init = true;
		}
		uboss_globalmq_push(queue); // 将消息压入全局消息队列
		if (ret) {
			uboss_error(ret, "LAUNCH %s %s", name, param ? param : "");
		}
		return ret;
	} else { // 返回错误
		uboss_error(ctx, "FAILED launch %s", name);
		uint32_t handle = ctx->handle;
		uboss_context_release(ctx);
		uboss_handle_retire(handle);
		struct drop_t d = { handle };
		uboss_mq_release(queue, drop_message, &d);
		return NULL;
	}
}

// 新建会话
int
uboss_context_newsession(struct uboss_context *ctx) {
	// session always be a positive number
	int session = ++ctx->session_id;
	if (session <= 0) { // 如果会话ID小于等于0
		ctx->session_id = 1; // 会话ID等于1
		return 1;
	}
	return session;
}

void 
uboss_context_grab(struct uboss_context *ctx) {
	ATOM_INC(&ctx->ref); // 原子操作 调用次数 加一
}

void
uboss_context_reserve(struct uboss_context *ctx) {
	uboss_context_grab(ctx);
	// don't count the context reserved, because uboss abort (the worker threads terminate) only when the total context is 0 .
	// the reserved context will be release at last.
	context_dec(); // 上下文结构数量减一
}

// 删除上下文结构
static void 
delete_context(struct uboss_context *ctx) {
	// 如果启动了日志文件
	if (ctx->logfile) {
		fclose(ctx->logfile); // 关闭日志句柄
	}
	uboss_module_instance_release(ctx->mod, ctx->instance); // 调用模块中的释放函数
	uboss_mq_mark_release(ctx->queue); // 标记 消息队列 可以释放
	CHECKCALLING_DESTROY(ctx) // 检查并销毁 上下文结构
	uboss_free(ctx); // 释放上下文结构
	context_dec(); // 上下文结构数量减一
}

// 释放上下文
struct uboss_context * 
uboss_context_release(struct uboss_context *ctx) {
	// 如果上下文结构数量减一，等于0
	if (ATOM_DEC(&ctx->ref) == 0) {
		delete_context(ctx); // 删除上下文结构
		return NULL;
	}
	return ctx;
}

// 将上下文压入消息队列
int
uboss_context_push(uint32_t handle, struct uboss_message *message) {
	struct uboss_context * ctx = uboss_handle_grab(handle);
	if (ctx == NULL) {
		return -1;
	}
	uboss_mq_push(ctx->queue, message); // 将消息压入消息队列
	uboss_context_release(ctx); // 释放上下文

	return 0;
}

void 
uboss_context_endless(uint32_t handle) {
	struct uboss_context * ctx = uboss_handle_grab(handle);
	if (ctx == NULL) {
		return;
	}
	ctx->endless = true;
	uboss_context_release(ctx);
}

// 是否为远程
int 
uboss_isremote(struct uboss_context * ctx, uint32_t handle, int * harbor) {
	int ret = uboss_harbor_message_isremote(handle);
	if (harbor) {
		*harbor = (int)(handle >> HANDLE_REMOTE_SHIFT);
	}
	return ret;
}

// 分发消息
static void
dispatch_message(struct uboss_context *ctx, struct uboss_message *msg) {
	assert(ctx->init);
	CHECKCALLING_BEGIN(ctx)
	pthread_setspecific(G_NODE.handle_key, (void *)(uintptr_t)(ctx->handle)); // 设置线程通道
	int type = msg->sz >> MESSAGE_TYPE_SHIFT; // 获得消息的类型
	size_t sz = msg->sz & MESSAGE_TYPE_MASK; // 获得消息的长度
	if (ctx->logfile) { // 如果uboss上下文设置类日志文件
		uboss_log_output(ctx->logfile, msg->source, type, msg->session, msg->data, sz); // 写入日志信息
	}
	++ctx->message_count;

	int reserve_msg;
	if (ctx->profile) {
		ctx->cpu_start = uboss_thread_time();
		reserve_msg = ctx->cb(ctx, ctx->cb_ud, type, msg->session, msg->source, msg->data, sz);
		uint64_t cost_time = uboss_thread_time() - ctx->cpu_start;
		ctx->cpu_cost += cost_time;
	} else {
		// 调用 服务模块中的返回函数
		reserve_msg = ctx->cb(ctx, ctx->cb_ud, type, msg->session, msg->source, msg->data, sz);
	}
	if (!reserve_msg) {
		uboss_free(msg->data); // 释放消息数据
	}
	CHECKCALLING_END(ctx)
}

// 分发所有消息
void 
uboss_context_dispatchall(struct uboss_context * ctx) {
	// for uboss_error
	struct uboss_message msg;
	struct message_queue *q = ctx->queue; // 取出上下文中的消息队列
	while (!uboss_mq_pop(q,&msg)) { // 弹出消息队列中的消息
		dispatch_message(ctx, &msg); // 将消息分发出去
	}
}

// 框架分发消息
struct message_queue * 
uboss_context_message_dispatch(struct uboss_monitor *sm, struct message_queue *q, int weight) {
	// 如果传入的消息队列地址为空值
	if (q == NULL) {
		q = uboss_globalmq_pop(); // 弹出全局消息
		if (q==NULL)
			return NULL;
	}

	uint32_t handle = uboss_mq_handle(q); // 从队列中取出对应的句柄值

	struct uboss_context * ctx = uboss_handle_grab(handle); // 根据 句柄值 获得上下文结构的指针
	if (ctx == NULL) {
		struct drop_t d = { handle };
		uboss_mq_release(q, drop_message, &d); // 释放消息队列
		return uboss_globalmq_pop(); // 弹出全局消息队列
	}

	int i,n=1;
	struct uboss_message msg;

	for (i=0;i<n;i++) {
		if (uboss_mq_pop(q,&msg)) { // 弹出uboss消息
			uboss_context_release(ctx); // 释放上下文
			return uboss_globalmq_pop(); // 从全局队列中弹出消息
		} else if (i==0 && weight >= 0) {
			n = uboss_mq_length(q); // 获得队列的长度
			n >>= weight; // 权重
		}

		// 服务消息队列调度时，如果发现过载，则打印输出过载信息
		// 过载后框架不会做其他处理，仅输出消息
		int overload = uboss_mq_overload(q); // 消息过载
		if (overload) {
			uboss_error(ctx, "May overload, message queue length = %d", overload);
		}

		
		//
		// 在处理这条消息前触发监视，给这个线程的版本加一
		// 并将消息来源和处理的服务地址发给监视线程。
		//
		uboss_monitor_trigger(sm, msg.source , handle); // 触发监视

		// 如果上下文中的返回函数指针为空
		if (ctx->cb == NULL) {
			uboss_free(msg.data); // 释放消息的数据内存空间
		} else {
			dispatch_message(ctx, &msg); // 核心功能：分发消息
		}

		//
		// 处理完消息后，再触发监视，将来源和目的设置成框架自己
		// 以做好下次调用准备，即清空来源和目的地址。
		//
		uboss_monitor_trigger(sm, 0,0); // 触发监视
	}

	assert(q == ctx->queue);
	struct message_queue *nq = uboss_globalmq_pop();
	if (nq) {
		// If global mq is not empty , push q back, and return next queue (nq)
		// Else (global mq is empty or block, don't push q back, and return q again (for next dispatch)
		uboss_globalmq_push(q);
		q = nq;
	} 
	uboss_context_release(ctx);

	return q;
}

static void
copy_name(char name[GLOBALNAME_LENGTH], const char * addr) {
	int i;
	for (i=0;i<GLOBALNAME_LENGTH && addr[i];i++) {
		name[i] = addr[i];
	}
	for (;i<GLOBALNAME_LENGTH;i++) {
		name[i] = '\0';
	}
}

// 根据名字请求
uint32_t 
uboss_queryname(struct uboss_context * context, const char * name) {
	switch(name[0]) { // 取出第一个字符
	case ':': // 冒号，为字符串名称，直接返回
		return strtoul(name+1,NULL,16);
	case '.': // 点，为数字型名称，需要操作后返回
		return uboss_handle_findname(name + 1);
	}
	uboss_error(context, "Don't support query global name %s",name);
	return 0;
}



// 将 uboss_command 相关代码移到新文件中

// 过滤参数
static void
_filter_args(struct uboss_context * context, int type, int *session, void ** data, size_t * sz) {
	int needcopy = !(type & PTYPE_TAG_DONTCOPY);
	int allocsession = type & PTYPE_TAG_ALLOCSESSION;
	type &= 0xff;

	// 允许会话
	if (allocsession) {
		assert(*session == 0);
		*session = uboss_context_newsession(context); // 新建会话
	}

	//  需要复制数据
	if (needcopy && *data) {
		char * msg = uboss_malloc(*sz+1); // 分配数据的内存空间
		memcpy(msg, *data, *sz); // 复制数据到内存空间
		msg[*sz] = '\0'; // 设置新数据的最后为0
		*data = msg; // 将新数据的地址复制给原来数据地址
	}

	// 重建数据长度的值
	*sz |= (size_t)type << MESSAGE_TYPE_SHIFT;
}

// 发送消息
int
uboss_send(struct uboss_context * context, uint32_t source, uint32_t destination , int type, int session, void * data, size_t sz) {
	if ((sz & MESSAGE_TYPE_MASK) != sz) {
		uboss_error(context, "The message to %x is too large", destination);
		if (type & PTYPE_TAG_DONTCOPY) {
			uboss_free(data); // 释放数据的内存空间
		}
		return -1;
	}
	_filter_args(context, type, &session, (void **)&data, &sz); // 过滤参数

	// 如果来源地址为0，表示服务自己
	if (source == 0) {
		source = context->handle;
	}

	// 如果目的地址为0，返回会话ID
	if (destination == 0) {
		return session;
	}
	if (uboss_harbor_message_isremote(destination)) {
		struct remote_message * rmsg = uboss_malloc(sizeof(*rmsg)); // 分配远程消息的内存空间
		rmsg->destination.handle = destination; // 目的地址
		rmsg->message = data; // 数据的地址
		rmsg->sz = sz; // 数据的长度
		uboss_harbor_send(rmsg, source, session); // 发送消息到集群中
	} else { // 本地消息
		struct uboss_message smsg;
		smsg.source = source;
		smsg.session = session;
		smsg.data = data;
		smsg.sz = sz;

		// 将消息压入消息队列
		if (uboss_context_push(destination, &smsg)) {
			uboss_free(data);
			return -1;
		}
	}
	return session;
}

// 以服务名字来发送消息
int
uboss_sendname(struct uboss_context * context, uint32_t source, const char * addr , int type, int session, void * data, size_t sz) {
	if (source == 0) {
		source = context->handle;
	}
	uint32_t des = 0;
	if (addr[0] == ':') {
		des = strtoul(addr+1, NULL, 16);
	} else if (addr[0] == '.') {
		des = uboss_handle_findname(addr + 1);
		if (des == 0) {
			if (type & PTYPE_TAG_DONTCOPY) {
				uboss_free(data);
			}
			return -1;
		}
	} else {
		_filter_args(context, type, &session, (void **)&data, &sz);

		struct remote_message * rmsg = uboss_malloc(sizeof(*rmsg));
		copy_name(rmsg->destination.name, addr);
		rmsg->destination.handle = 0;
		rmsg->message = data;
		rmsg->sz = sz;

		uboss_harbor_send(rmsg, source, session);
		return session;
	}

	return uboss_send(context, source, des, type, session, data, sz);
}

// 返回上下文结构中的句柄值
uint32_t 
uboss_context_handle(struct uboss_context *ctx) {
	return ctx->handle;
}

// 注册服务模块中的返回函数
void 
uboss_callback(struct uboss_context * context, void *ud, uboss_cb cb) {
	context->cb = cb; // 返回函数的指针
	context->cb_ud = ud; // 用户的数据指针
}

// 发送消息
void
uboss_context_send(struct uboss_context * ctx, void * msg, size_t sz, uint32_t source, int type, int session) {
	struct uboss_message smsg; // 声明消息结构
	smsg.source = source; // 来源地址
	smsg.session = session; // 目的地址
	smsg.data = msg; // 消息的指针
	smsg.sz = sz | (size_t)type << MESSAGE_TYPE_SHIFT; // 消息的长度

	uboss_mq_push(ctx->queue, &smsg); // 将消息压入消息队列
}

// 全局线程管道初始化
void 
uboss_globalinit(void) {
	G_NODE.total = 0;
	G_NODE.monitor_exit = 0;
	G_NODE.init = 1;
	if (pthread_key_create(&G_NODE.handle_key, NULL)) {
		fprintf(stderr, "pthread_key_create failed");
		exit(1);
	}
	// set mainthread's key
	uboss_initthread(THREAD_MAIN); // 初始化主线程
}

// 退出全局线程管道
void 
uboss_globalexit(void) {
	pthread_key_delete(G_NODE.handle_key);
}

// 初始化线程管道
void
uboss_initthread(int m) {
	uintptr_t v = (uint32_t)(-m);
	pthread_setspecific(G_NODE.handle_key, (void *)v);
}

