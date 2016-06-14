/*
** Copyright (c) 2014-2016 uboss.org All rights Reserved.
** uBoss - A lightweight micro service framework
**
** uBoss Gate Module
**
** Dali Wang<dali@uboss.org>
** See Copyright Notice in uboss.h
*/

#include "uboss.h"
#include "uboss_socket.h"
#include "databuffer.h"
#include "hashid.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>

#define BACKLOG 32

// 连接
struct connection {
	int id;	// uboss_socket id
	uint32_t agent; // 代理
	uint32_t client; // 客户端
	char remote_name[32]; // 远程名字
	struct databuffer buffer; // 数据缓冲区
};

// 网关的结构
struct gate {
	struct uboss_context *ctx; // uboss上下文结构
	int listen_id; // 监听ID
	uint32_t watchdog; // 看门狗
	uint32_t broker; // 经纪人（处理不同连接上的所有数据）
	int client_tag; // 客户端标志
	int header_size; // 头部大小
	int max_connection; // 最大连接数
	struct hashid hash; // 散列
	struct connection *conn; // 连接
	// todo: 释放保存的消息池指针 save message pool ptr for release
	struct messagepool mp; // 消息池
};

// 创建网关
struct gate *
gate_create(void) {
	struct gate * g = uboss_malloc(sizeof(*g)); // 分配内存空间
	memset(g,0,sizeof(*g)); // 清空内存空间
	g->listen_id = -1; // 监听ID = -1
	return g; // 返回 gate
}

// 释放网关
void
gate_release(struct gate *g) {
	int i;
	struct uboss_context *ctx = g->ctx; // 获得 uboss 上下文结构

	// 循环所有连接
	for (i=0;i<g->max_connection;i++) {
		struct connection *c = &g->conn[i]; // 取出连接
		if (c->id >=0) {
			uboss_socket_close(ctx, c->id); // 关闭 socket 连接
		}
	}
	if (g->listen_id >= 0) {
		uboss_socket_close(ctx, g->listen_id); // 关闭 socket 连接
	}
	messagepool_free(&g->mp); // 释放消息池
	hashid_clear(&g->hash); // 清理散列
	uboss_free(g->conn); // 释放连接
	uboss_free(g); // 释放结构
}

// 处理参数
static void
_parm(char *msg, int sz, int command_sz) {
	// 当长度大于命令长度时，在命令尾插入空格符
	while (command_sz < sz) {
		if (msg[command_sz] != ' ')
			break;
		++command_sz;
	}

	// 循环得到命令的每个字符
	int i;
	for (i=command_sz;i<sz;i++) {
		msg[i-command_sz] = msg[i];
	}
	msg[i-command_sz] = '\0'; // 结束
}

// 转发代理
static void
_forward_agent(struct gate * g, int fd, uint32_t agentaddr, uint32_t clientaddr) {
	// 查找散列的ID
	int id = hashid_lookup(&g->hash, fd);
	if (id >=0) {
		struct connection * agent = &g->conn[id]; // 获得连接
		agent->agent = agentaddr; // 获得代理的地址
		agent->client = clientaddr; // 获得客户端的地址
	}
}

// 执行文本指令
static void
_ctrl(struct gate * g, const void * msg, int sz) {
	struct uboss_context * ctx = g->ctx;
	char tmp[sz+1];
	memcpy(tmp, msg, sz);
	tmp[sz] = '\0';
	char * command = tmp;
	int i;
	if (sz == 0)
		return;
	for (i=0;i<sz;i++) {
		if (command[i]==' ') {
			break;
		}
	}

	// 踢掉
	if (memcmp(command,"kick",i)==0) {
		_parm(tmp, sz, i); // 处理参数
		int uid = strtol(command , NULL, 10);
		int id = hashid_lookup(&g->hash, uid); // 查找ID
		if (id>=0) {
			uboss_socket_close(ctx, uid); // 关闭 socket
		}
		return;
	}

	// 转发
	if (memcmp(command,"forward",i)==0) {
		_parm(tmp, sz, i); // 处理参数
		char * client = tmp;
		char * idstr = strsep(&client, " ");
		if (client == NULL) {
			return;
		}
		int id = strtol(idstr , NULL, 10);
		char * agent = strsep(&client, " ");
		if (client == NULL) {
			return;
		}
		uint32_t agent_handle = strtoul(agent+1, NULL, 16);
		uint32_t client_handle = strtoul(client+1, NULL, 16);
		_forward_agent(g, id, agent_handle, client_handle);
		return;
	}

	// 经纪人模式
	if (memcmp(command,"broker",i)==0) {
		_parm(tmp, sz, i); // 处理参数
		g->broker = uboss_queryname(ctx, command); // 按服务名查找
		return;
	}

	// 开始
	if (memcmp(command,"start",i) == 0) {
		_parm(tmp, sz, i); // 处理参数
		int uid = strtol(command , NULL, 10);
		int id = hashid_lookup(&g->hash, uid); // 查找散列ID
		if (id>=0) {
			uboss_socket_start(ctx, uid); // 开始 socket
		}
		return;
	}

	// 关闭
	if (memcmp(command, "close", i) == 0) {
		if (g->listen_id >= 0) {
			uboss_socket_close(ctx, g->listen_id); // 关闭 socket
			g->listen_id = -1;
		}
		return;
	}
	uboss_error(ctx, "[gate] Unkown command : %s", command);
}

// 报告
static void
_report(struct gate * g, const char * data, ...) {
	if (g->watchdog == 0) {
		return;
	}
	struct uboss_context * ctx = g->ctx; // 获得 uboss 上下文结构
	va_list ap;
	va_start(ap, data);
	char tmp[1024];
	int n = vsnprintf(tmp, sizeof(tmp), data, ap); // 复制数据
	va_end(ap);

	// 发送 uboss 文本消息
	uboss_send(ctx, 0, g->watchdog, PTYPE_TEXT,  0, tmp, n);
}

// 转发
static void
_forward(struct gate *g, struct connection * c, int size) {
	struct uboss_context * ctx = g->ctx;
	// 发送给经纪人
	if (g->broker) {
		void * temp = uboss_malloc(size); // 分配内存空间
		databuffer_read(&c->buffer,&g->mp,temp, size); // 从数据缓冲区读取数据到临时空间

		// 发送uboss消息
		uboss_send(ctx, 0, g->broker, g->client_tag | PTYPE_TAG_DONTCOPY, 0, temp, size); 
		return;
	}
	// 发送给客户的代理
	if (c->agent) {
		void * temp = uboss_malloc(size); // 分配内存空间
		databuffer_read(&c->buffer,&g->mp,temp, size); // 从数据缓冲区读取数据到临时空间

		// 发送uboss消息
		uboss_send(ctx, c->client, c->agent, g->client_tag | PTYPE_TAG_DONTCOPY, 0 , temp, size);
	} else if (g->watchdog) { // 发送给看门狗
		char * tmp = uboss_malloc(size + 32); // 分配内存空间
		int n = snprintf(tmp,32,"%d data ",c->id);
		databuffer_read(&c->buffer,&g->mp,tmp+n,size); // 从数据缓冲区读取数据到临时空间

		// 发送uboss消息
		uboss_send(ctx, 0, g->watchdog, PTYPE_TEXT | PTYPE_TAG_DONTCOPY, 0, tmp, size + n);
	}
}

// 消息调度
static void
dispatch_message(struct gate *g, struct connection *c, int id, void * data, int sz) {
	// 压入数据
	databuffer_push(&c->buffer,&g->mp, data, sz);
	for (;;) {
		// 读数据的头部
		int size = databuffer_readheader(&c->buffer, &g->mp, g->header_size);
		if (size < 0) {
			return;
		} else if (size > 0) {
			// 数据大于 16M
			if (size >= 0x1000000) {
				struct uboss_context * ctx = g->ctx;
				databuffer_clear(&c->buffer,&g->mp); // 清理数据缓冲区
				uboss_socket_close(ctx, id); // 关闭 socket
				uboss_error(ctx, "Recv socket message > 16M");
				return;
			} else {
				_forward(g, c, size); // 转寄
				databuffer_reset(&c->buffer); // 重置数据缓冲区
			}
		}
	}
}

// socket 消息调度
static void
dispatch_socket_message(struct gate *g, const struct uboss_socket_message * message, int sz) {
	struct uboss_context * ctx = g->ctx;
	switch(message->type) {
	// 数据
	case UBOSS_SOCKET_TYPE_DATA: {
		int id = hashid_lookup(&g->hash, message->id); // 查找散列ID
		if (id>=0) {
			struct connection *c = &g->conn[id]; // 获得连接
			dispatch_message(g, c, message->id, message->buffer, message->ud); // 消息调度
		} else {
			uboss_error(ctx, "Drop unknown connection %d message", message->id);
			uboss_socket_close(ctx, message->id); // 关闭 socket
			uboss_free(message->buffer); // 释放缓冲区指针
		}
		break;
	}
	// 连接
	case UBOSS_SOCKET_TYPE_CONNECT: {
		if (message->id == g->listen_id) {
			// start listening
			break;
		}
		int id = hashid_lookup(&g->hash, message->id); // 查找散列ID
		if (id<0) {
			uboss_error(ctx, "Close unknown connection %d", message->id);
			uboss_socket_close(ctx, message->id); // 关闭 socket
		}
		break;
	}
	// 关闭或错误
	case UBOSS_SOCKET_TYPE_CLOSE:
	case UBOSS_SOCKET_TYPE_ERROR: {
		int id = hashid_remove(&g->hash, message->id); // 移除散列ID
		if (id>=0) {
			struct connection *c = &g->conn[id]; // 获得连接
			databuffer_clear(&c->buffer,&g->mp); // 清除缓冲区
			memset(c, 0, sizeof(*c)); // 清空内存
			c->id = -1;
			_report(g, "%d close", message->id); // 调用 report 函数
		}
		break;
	}
	// 接受
	case UBOSS_SOCKET_TYPE_ACCEPT:
		// report accept, then it will be get a UBOSS_SOCKET_TYPE_CONNECT message
		assert(g->listen_id == message->id);
		if (hashid_full(&g->hash)) {
			uboss_socket_close(ctx, message->ud); // 关闭 socket
		} else {
			struct connection *c = &g->conn[hashid_insert(&g->hash, message->ud)]; // 获得连接
			if (sz >= sizeof(c->remote_name)) {
				sz = sizeof(c->remote_name) - 1;
			}
			c->id = message->ud;
			memcpy(c->remote_name, message+1, sz); // 复制远程名字
			c->remote_name[sz] = '\0';
			_report(g, "%d open %d %s:0",c->id, c->id, c->remote_name); // 调用 report 函数
			uboss_error(ctx, "socket open: %x", c->id);
		}
		break;
	// 警告
	case UBOSS_SOCKET_TYPE_WARNING:
		uboss_error(ctx, "fd (%d) send buffer (%d)K", message->id, message->ud);
		break;
	}
}

// 回调函数
static int
_cb(struct uboss_context * ctx, void * ud, int type, int session, uint32_t source, const void * msg, size_t sz) {
	struct gate *g = ud;
	switch(type) {
	// 文本
	case PTYPE_TEXT:
		_ctrl(g , msg , (int)sz); // 执行文本指令
		break;
	// 客户端
	case PTYPE_CLIENT: {
		if (sz <=4 ) {
			uboss_error(ctx, "Invalid client message from %x",source);
			break;
		}
		// The last 4 bytes in msg are the id of socket, write following bytes to it
		const uint8_t * idbuf = msg + sz - 4;
		uint32_t uid = idbuf[0] | idbuf[1] << 8 | idbuf[2] << 16 | idbuf[3] << 24;
		int id = hashid_lookup(&g->hash, uid); // 查找散列ID
		if (id>=0) {
			// don't send id (last 4 bytes)
			uboss_socket_send(ctx, uid, (void*)msg, sz-4); // 发送 socket 消息
			// return 1 means don't free msg
			return 1;
		} else {
			uboss_error(ctx, "Invalid client id %d from %x",(int)uid,source);
			break;
		}
	}
	// 网络
	case PTYPE_SOCKET:
		// recv socket message from uboss_socket
		dispatch_socket_message(g, msg, (int)(sz-sizeof(struct uboss_socket_message)));
		break;
	}
	return 0;
}

// 启动监听
static int
start_listen(struct gate *g, char * listen_addr) {
	struct uboss_context * ctx = g->ctx; // 获得 uboss 上下文结构
	char * portstr = strchr(listen_addr,':'); // 在监听地址中查找 ':' 字符
	const char * host = "";
	int port;
	// 如果找不到':'，则将整个地址转成端口
	if (portstr == NULL) {
		port = strtol(listen_addr, NULL, 10); // 获得监听的端口
		// 判断端口是否小于等于零
		if (port <= 0) {
			uboss_error(ctx, "Invalid gate address %s",listen_addr);
			return 1;
		}
	} else { // 否则，单独转换端口
		port = strtol(portstr + 1, NULL, 10); // 获得监听的端口
		if (port <= 0) {
			uboss_error(ctx, "Invalid gate address %s",listen_addr);
			return 1;
		}
		portstr[0] = '\0'; // 将 ':' 替换成 \0 截断字符串
		host = listen_addr; // 获得监听的 IP 地址
	}
	g->listen_id = uboss_socket_listen(ctx, host, port, BACKLOG); // 监听 socket （积压模式）
	if (g->listen_id < 0) {
		return 1;
	}
	uboss_socket_start(ctx, g->listen_id); // 启动 socket
	return 0;
}

// 初始化网关
int
gate_init(struct gate *g , struct uboss_context * ctx, char * parm) {
	// 如果参数为空，返回错误
	if (parm == NULL)
		return 1;

	int max = 0;
	int sz = strlen(parm)+1; // 获得参数字符串的长度
	char watchdog[sz]; // 看门狗
	char binding[sz]; // 绑定监听IP:Port
	int client_tag = 0; // 客户端标志
	char header; // 头部

	// 解析参数字符串
	int n = sscanf(parm, "%c %s %s %d %d", &header, watchdog, binding, &client_tag, &max);
	// n小于4，表示参数的数量错误
	if (n<4) {
		uboss_error(ctx, "Invalid gate parm %s",parm);
		return 1;
	}
	if (max <=0 ) {
		uboss_error(ctx, "Need max connection");
		return 1;
	}

	// 如果头部不为'S' 或 'L'，则错误
	if (header != 'S' && header !='L') {
		uboss_error(ctx, "Invalid data header style");
		return 1;
	}

	// 客户端标志
	if (client_tag == 0) {
		client_tag = PTYPE_CLIENT;
	}

	// 看门狗的第一个字符为 '!'
	if (watchdog[0] == '!') {
		g->watchdog = 0;
	} else {
		g->watchdog = uboss_queryname(ctx, watchdog); // 按名字查找看门狗服务
		// 没找到服务
		if (g->watchdog == 0) {
			uboss_error(ctx, "Invalid watchdog %s",watchdog);
			return 1;
		}
	}

	g->ctx = ctx;

	// 初始化散列ID
	hashid_init(&g->hash, max);
	g->conn = uboss_malloc(max * sizeof(struct connection)); // 分配内存空间
	memset(g->conn, 0, max *sizeof(struct connection)); // 清空内存空间
	g->max_connection = max; // 设置最大连接数
	int i;
	for (i=0;i<max;i++) {
		g->conn[i].id = -1;
	}
	
	g->client_tag = client_tag;
	g->header_size = header=='S' ? 2 : 4;

	// 设置回调函数
	uboss_callback(ctx,g,_cb);

	// 调用启动监听，并返回结果
	return start_listen(g,binding);
}
