#include "uboss.h"

#include "uboss_server.h"
#include "uboss_module.h"
#include "uboss_handle.h"
#include "uboss_mq.h"
#include "uboss_timer.h"
#include "uboss_harbor.h"
#include "uboss_env.h"
#include "uboss_monitor.h"
#include "uboss_imp.h"
#include "uboss_log.h"
#include "spinlock.h"
#include "atomic.h"

#include <pthread.h>

#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#ifdef CALLING_CHECK

#define CHECKCALLING_BEGIN(ctx) if (!(spinlock_trylock(&ctx->calling))) { assert(0); }
#define CHECKCALLING_END(ctx) spinlock_unlock(&ctx->calling);
#define CHECKCALLING_INIT(ctx) spinlock_init(&ctx->calling);
#define CHECKCALLING_DESTROY(ctx) spinlock_destroy(&ctx->calling);
#define CHECKCALLING_DECL struct spinlock calling;

#else

#define CHECKCALLING_BEGIN(ctx)
#define CHECKCALLING_END(ctx)
#define CHECKCALLING_INIT(ctx)
#define CHECKCALLING_DESTROY(ctx)
#define CHECKCALLING_DECL

#endif

// uBoss 上下文结构
struct uboss_context {
	void * instance; // 实例
	struct uboss_module * mod; // 模块
	void * cb_ud; // 用户数据的指针
	uboss_cb cb; // 服务模块的返回函数指针
	struct message_queue *queue; // 消息队列
	FILE * logfile; // 日志文件流
	char result[32]; // 结果
	uint32_t handle; // 句柄值
	int session_id; // 会话 ID
	int ref; // 调用次数
	bool init; // 初始化
	bool endless; // 终结标志

	CHECKCALLING_DECL
};

// uBoss 的节点
struct uboss_node {
	int total; // 节点总数
	int init;
	uint32_t monitor_exit;
	pthread_key_t handle_key;
};

// 声明全局节点
static struct uboss_node G_NODE;

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

// ID 转换为 十六进制
static void
id_to_hex(char * str, uint32_t id) {
	int i;
	static char hex[16] = { '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F' };
	str[0] = ':';
	for (i=0;i<8;i++) {
		str[i+1] = hex[(id >> ((7-i) * 4))&0xf];
	}
	str[9] = '\0';
}

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
		struct uboss_context * ret = uboss_context_release(ctx); // 从模块指针中获取 释放函数的指针
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
	context_dec();
}

// 删除上下文结构
static void 
delete_context(struct uboss_context *ctx) {
	if (ctx->logfile) {
		fclose(ctx->logfile);
	}
	uboss_module_instance_release(ctx->mod, ctx->instance);
	uboss_mq_mark_release(ctx->queue);
	CHECKCALLING_DESTROY(ctx)
	uboss_free(ctx);
	context_dec();
}

// 释放上下文
struct uboss_context * 
uboss_context_release(struct uboss_context *ctx) {
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

	// 调用 服务模块中的返回函数
	if (!ctx->cb(ctx, ctx->cb_ud, type, msg->session, msg->source, msg->data, sz)) {
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
		int overload = uboss_mq_overload(q); // 消息过载
		if (overload) {
			uboss_error(ctx, "May overload, message queue length = %d", overload);
		}

		uboss_monitor_trigger(sm, msg.source , handle); // 触发监视

		// 如果上下文中的返回函数指针为空
		if (ctx->cb == NULL) {
			uboss_free(msg.data); // 释放消息的数据内存空间
		} else {
			dispatch_message(ctx, &msg); // 分发消息
		}

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

// 退出句柄
static void
handle_exit(struct uboss_context * context, uint32_t handle) {
	if (handle == 0) { // 如果句柄为 "0" 即为自己
		handle = context->handle; // 从上下文获得句柄
		uboss_error(context, "KILL self"); // 杀死自己
	} else {
		uboss_error(context, "KILL :%0x", handle); // 杀死此句柄的服务
	}
	if (G_NODE.monitor_exit) {
		uboss_send(context,  handle, G_NODE.monitor_exit, PTYPE_CLIENT, 0, NULL, 0); // 发送消息
	}
	uboss_handle_retire(handle); // 回收句柄
}

// uboss command
// 命令式
struct command_func {
	const char *name;
	const char * (*func)(struct uboss_context * context, const char * param);
};

// 超时指令
static const char *
cmd_timeout(struct uboss_context * context, const char * param) {
	char * session_ptr = NULL;
	int ti = strtol(param, &session_ptr, 10);
	int session = uboss_context_newsession(context);
	uboss_timeout(context->handle, ti, session);
	sprintf(context->result, "%d", session);
	return context->result;
}

// 注册指令
static const char *
cmd_reg(struct uboss_context * context, const char * param) {
	if (param == NULL || param[0] == '\0') {
		sprintf(context->result, ":%x", context->handle); // 打印句柄到返回结果
		return context->result;
	} else if (param[0] == '.') { // 如果参数第一个字节为 "." 时，执行：
		return uboss_handle_namehandle(context->handle, param + 1); // 关联名字于句柄值
	} else {
		uboss_error(context, "Can't register global name %s in C", param);
		return NULL;
	}
}

// 查询指令
static const char *
cmd_query(struct uboss_context * context, const char * param) {
	if (param[0] == '.') { // 当第一个字节为 "." 时，执行：
		uint32_t handle = uboss_handle_findname(param+1); // 根据名字查找句柄值
		if (handle) {
			sprintf(context->result, ":%x", handle); // 打印句柄到返回结果
			return context->result;
		}
	}
	return NULL;
}

// 名字指令
static const char *
cmd_name(struct uboss_context * context, const char * param) {
	int size = strlen(param); // 计算参数的长度
	char name[size+1];
	char handle[size+1];
	sscanf(param,"%s %s",name,handle); // 扫描参数，生成名字和句柄
	if (handle[0] != ':') { // 如果句柄第一个字节不是冒号时
		return NULL; // 返回空
	}
	uint32_t handle_id = strtoul(handle+1, NULL, 16);
	if (handle_id == 0) {
		return NULL;
	}
	if (name[0] == '.') { // 如果服务名字的第一个字节是 "." 时
		return uboss_handle_namehandle(handle_id, name + 1); // 关联名字于句柄值
	} else {
		uboss_error(context, "Can't set global name %s in C", name);
	}
	return NULL;
}

// 退出指令
static const char *
cmd_exit(struct uboss_context * context, const char * param) {
	handle_exit(context, 0); // 退出回收句柄
	return NULL;
}

// 转换为句柄
static uint32_t
tohandle(struct uboss_context * context, const char * param) {
	uint32_t handle = 0;
	if (param[0] == ':') { // 如果句柄为 ":"
		handle = strtoul(param+1, NULL, 16); // 字符串转换整数
	} else if (param[0] == '.') { // 如果服务名为 "."
		handle = uboss_handle_findname(param+1); // 根据名字查找句柄值
	} else {
		uboss_error(context, "Can't convert %s to handle",param);
	}

	return handle; // 返回句柄
}

// 杀死指令
static const char *
cmd_kill(struct uboss_context * context, const char * param) {
	uint32_t handle = tohandle(context, param); // 转换为句柄
	if (handle) {
		handle_exit(context, handle); // 退出回收句柄
	}
	return NULL;
}

// 开始指令
static const char *
cmd_launch(struct uboss_context * context, const char * param) {
	size_t sz = strlen(param); // 计算参数的长度
	char tmp[sz+1];
	strcpy(tmp,param); // 复制参数到临时字符串
	char * args = tmp;
	char * mod = strsep(&args, " \t\r\n");
	args = strsep(&args, "\r\n");
	struct uboss_context * inst = uboss_context_new(mod,args); // 新建上下文
	if (inst == NULL) {
		return NULL;
	} else {
		id_to_hex(context->result, inst->handle); // ID 转 16进制
		return context->result;
	}
}

// 获得环境指令
static const char *
cmd_getenv(struct uboss_context * context, const char * param) {
	return uboss_getenv(param); // 获得 lua 环境的变量值
}

// 设置环境指令
static const char *
cmd_setenv(struct uboss_context * context, const char * param) {
	size_t sz = strlen(param); // 计算参数的长度
	char key[sz+1];
	int i;
	for (i=0;param[i] != ' ' && param[i];i++) {
		key[i] = param[i];
	}
	if (param[i] == '\0')
		return NULL;

	key[i] = '\0';
	param += i+1;
	
	uboss_setenv(key,param); // 设置 lua 环境的变量值
	return NULL;
}

// 开始时间指令
static const char *
cmd_starttime(struct uboss_context * context, const char * param) {
	uint32_t sec = uboss_starttime(); // 获得开始时间的秒数
	sprintf(context->result,"%u",sec); // 打印到返回字符串中
	return context->result;
}

// 设置终结服务指令
static const char *
cmd_endless(struct uboss_context * context, const char * param) {
	if (context->endless) { // 如果有终结指令存在
		strcpy(context->result, "1"); // 打印 "1" 到返回字符串中
		context->endless = false;
		return context->result;
	}
	return NULL;
}

// 终止指令
static const char *
cmd_abort(struct uboss_context * context, const char * param) {
	uboss_handle_retireall(); // 回收所有句柄
	return NULL;
}

// 监视器指令
static const char *
cmd_monitor(struct uboss_context * context, const char * param) {
	uint32_t handle=0;
	if (param == NULL || param[0] == '\0') {
		if (G_NODE.monitor_exit) {
			// return current monitor serivce
			sprintf(context->result, ":%x", G_NODE.monitor_exit);
			return context->result;
		}
		return NULL;
	} else {
		handle = tohandle(context, param);
	}
	G_NODE.monitor_exit = handle;
	return NULL;
}

// 获得消息队列长度的指令
static const char *
cmd_mqlen(struct uboss_context * context, const char * param) {
	int len = uboss_mq_length(context->queue); // 计算消息队列的长度
	sprintf(context->result, "%d", len); // 转换为字符串
	return context->result; // 返回 长度字符串
}

// 打开日志的指令
static const char *
cmd_logon(struct uboss_context * context, const char * param) {
	uint32_t handle = tohandle(context, param);
	if (handle == 0)
		return NULL;
	struct uboss_context * ctx = uboss_handle_grab(handle);
	if (ctx == NULL)
		return NULL;
	FILE *f = NULL;
	FILE * lastf = ctx->logfile;
	if (lastf == NULL) {
		f = uboss_log_open(context, handle); // 打开日志
		if (f) {
			if (!ATOM_CAS_POINTER(&ctx->logfile, NULL, f)) {
				// logfile opens in other thread, close this one.
				fclose(f);
			}
		}
	}
	uboss_context_release(ctx);
	return NULL;
}

// 关闭日志的指令
static const char *
cmd_logoff(struct uboss_context * context, const char * param) {
	uint32_t handle = tohandle(context, param);
	if (handle == 0)
		return NULL;
	struct uboss_context * ctx = uboss_handle_grab(handle);
	if (ctx == NULL)
		return NULL;
	FILE * f = ctx->logfile;
	if (f) {
		// logfile may close in other thread
		if (ATOM_CAS_POINTER(&ctx->logfile, f, NULL)) {
			uboss_log_close(context, f, handle); // 关闭日志
		}
	}
	uboss_context_release(ctx);
	return NULL;
}

// 信号的指令
static const char *
cmd_signal(struct uboss_context * context, const char * param) {
	uint32_t handle = tohandle(context, param);
	if (handle == 0)
		return NULL;
	struct uboss_context * ctx = uboss_handle_grab(handle);
	if (ctx == NULL)
		return NULL;
	param = strchr(param, ' ');
	int sig = 0;
	if (param) {
		sig = strtol(param, NULL, 0);
	}
	// NOTICE: the signal function should be thread safe.
	uboss_module_instance_signal(ctx->mod, ctx->instance, sig);

	uboss_context_release(ctx);
	return NULL;
}

// 指令列表
static struct command_func cmd_funcs[] = {
	{ "TIMEOUT", cmd_timeout },
	{ "REG", cmd_reg },
	{ "QUERY", cmd_query },
	{ "NAME", cmd_name },
	{ "EXIT", cmd_exit },
	{ "KILL", cmd_kill },
	{ "LAUNCH", cmd_launch },
	{ "GETENV", cmd_getenv },
	{ "SETENV", cmd_setenv },
	{ "STARTTIME", cmd_starttime },
	{ "ENDLESS", cmd_endless },
	{ "ABORT", cmd_abort },
	{ "MONITOR", cmd_monitor },
	{ "MQLEN", cmd_mqlen },
	{ "LOGON", cmd_logon },
	{ "LOGOFF", cmd_logoff },
	{ "SIGNAL", cmd_signal },
	{ NULL, NULL },
};

// 调用命令
const char * 
uboss_command(struct uboss_context * context, const char * cmd , const char * param) {
	struct command_func * method = &cmd_funcs[0]; // 取出命令函数的结构第一个指针
	while(method->name) { // 取出方法的名字，并循环
		if (strcmp(cmd, method->name) == 0) { // 比较
			return method->func(context, param); // 调用回调函数
		}
		++method; // 取下一个指针
	}

	return NULL;
}

static void
_filter_args(struct uboss_context * context, int type, int *session, void ** data, size_t * sz) {
	int needcopy = !(type & PTYPE_TAG_DONTCOPY);
	int allocsession = type & PTYPE_TAG_ALLOCSESSION;
	type &= 0xff;

	if (allocsession) {
		assert(*session == 0);
		*session = uboss_context_newsession(context);
	}

	if (needcopy && *data) {
		char * msg = uboss_malloc(*sz+1);
		memcpy(msg, *data, *sz);
		msg[*sz] = '\0';
		*data = msg;
	}

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

// 全局初始化
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

// 
void 
uboss_globalexit(void) {
	pthread_key_delete(G_NODE.handle_key);
}

// 初始化线程
void
uboss_initthread(int m) {
	uintptr_t v = (uint32_t)(-m);
	pthread_setspecific(G_NODE.handle_key, (void *)v);
}

