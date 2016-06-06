#include "uboss.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// 定义结构
struct monitor {
	int a; 
};

// 回调函数
static int
_cb(struct uboss_context * context, void *ud, int type, int session, uint32_t source , const void * msg, size_t sz) {
	return 0;
}

// 初始化
int
monitor_init(struct monitor *m, struct uboss_context *ctx, const char * args) {
	uboss_callback(ctx, m , _cb); // 设置回调函数
	return 0;
}

// 创建
struct monitor *
monitor_create(void) {
	struct monitor *tmp = uboss_malloc(sizeof(*tmp));
	tmp->a = 1;
	return tmp;
}

// 释放
void
monitor_release(struct monitor *m) {

}

// 信号
void
monitor_signal(struct monitor *m, int signal) {

}
