#include "uboss.h"

#include "uboss_monitor.h"
#include "uboss_server.h"
#include "uboss.h"
#include "atomic.h"

#include <stdlib.h>
#include <string.h>

// 监视器的结构
struct uboss_monitor {
	int version; // 版本
	int check_version; // 检查版本
	uint32_t source; // 来源地址
	uint32_t destination; // 目的地址
};

// 新建监视器
struct uboss_monitor * 
uboss_monitor_new() {
	struct uboss_monitor * ret = uboss_malloc(sizeof(*ret)); // 分配内存空间
	memset(ret, 0, sizeof(*ret)); // 清空内存空间
	return ret; // 返回指针
}

// 删除监视器
void 
uboss_monitor_delete(struct uboss_monitor *sm) {
	uboss_free(sm); // 释放监视器的结构
}

// 触发监视器
void 
uboss_monitor_trigger(struct uboss_monitor *sm, uint32_t source, uint32_t destination) {
	sm->source = source; // 来源地址
	sm->destination = destination; // 目的地址
	ATOM_INC(&sm->version); // 版本号加一
}

// 检查监视器
void 
uboss_monitor_check(struct uboss_monitor *sm) {
	if (sm->version == sm->check_version) { // 比较版本和检查版本的值
		if (sm->destination) { // 目的地址
			uboss_context_endless(sm->destination); // 结束目标服务
			uboss_error(NULL, "A message from [ :%08x ] to [ :%08x ] maybe in an endless loop (version = %d)", sm->source , sm->destination, sm->version);
		}
	} else {
		sm->check_version = sm->version; // 设置版本和检查版本的值
	}
}
