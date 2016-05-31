#include "uboss.h"
#include "uboss_mq.h"
#include "uboss_handle.h"
#include "uboss_lock.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

#define DEFAULT_QUEUE_SIZE 64 // 默认队列大小
#define MAX_GLOBAL_MQ 0x10000 // 全局消息队列最大值

// 0 means mq is not in global mq.
// 1 means mq is in global mq , or the message is dispatching.

#define MQ_IN_GLOBAL 1
#define MQ_OVERLOAD 1024

// 消息队列的结构
struct message_queue {
	struct spinlock lock; // 锁
	uint32_t handle; // 句柄值
	int cap; // 链表数量
	int head; // 链表头
	int tail; // 链表尾
	int release; // 是否可释放
	int in_global; // 是否在全局队列
	int overload; // 过载
	int overload_threshold; // 过载阀值
	struct uboss_message *queue;  // 消息队列的指针
	struct message_queue *next;  // 下一个消息队列的指针
};

// 全局消息队列的结构
struct global_queue {
	struct message_queue *head; // 消息队列的链表头
	struct message_queue *tail; // 消息队列的链表尾
	struct spinlock lock; // 锁
};

static struct global_queue *Q = NULL; // 声明全局队列

// 将服务的消息队列压入全局消息队列
void 
uboss_globalmq_push(struct message_queue * queue) {
	struct global_queue *q= Q; // 获取 全局消息队列

	SPIN_LOCK(q) // 锁住
	assert(queue->next == NULL);
	if(q->tail) {
		q->tail->next = queue;
		q->tail = queue;
	} else {
		q->head = q->tail = queue;
	}
	SPIN_UNLOCK(q) // 解锁
}

// 从全局消息队列中弹出服务的消息队列
struct message_queue * 
uboss_globalmq_pop() {
	struct global_queue *q = Q; // 声明全局消息队列

	SPIN_LOCK(q) // 锁住
	struct message_queue *mq = q->head;
	if(mq) {
		q->head = mq->next;
		if(q->head == NULL) {
			assert(mq == q->tail);
			q->tail = NULL;
		}
		mq->next = NULL;
	}
	SPIN_UNLOCK(q) // 解锁

	return mq;
}

// 创建服务的消息队列
struct message_queue * 
uboss_mq_create(uint32_t handle) {
	struct message_queue *q = uboss_malloc(sizeof(*q)); // 分配消息队列的内存空间
	q->handle = handle; // 句柄值
	q->cap = DEFAULT_QUEUE_SIZE; // 默认队列大小
	q->head = 0; // 链表的头
	q->tail = 0; // 链表的尾
	SPIN_INIT(q) // 初始化锁
	// When the queue is create (always between service create and service init) ,
	// set in_global flag to avoid push it to global queue .
	// If the service init success, uboss_context_new will call uboss_mq_push to push it to global queue.
	q->in_global = MQ_IN_GLOBAL; // 是否在全局队列中
	q->release = 0; // 是否可释放
	q->overload = 0; // 是否过载
	q->overload_threshold = MQ_OVERLOAD; // 过载的阀值
	q->queue = uboss_malloc(sizeof(struct uboss_message) * q->cap); // 分配队列的内存空间
	q->next = NULL; // 下一个队列的指针

	return q;
}

// 释放
static void 
_release(struct message_queue *q) {
	assert(q->next == NULL);
	SPIN_DESTROY(q)
	uboss_free(q->queue);
	uboss_free(q);
}

// 从服务的消息队列中获取句柄的值
uint32_t 
uboss_mq_handle(struct message_queue *q) {
	return q->handle; // 返回句柄
}

// 获得消息队列的长度
int
uboss_mq_length(struct message_queue *q) {
	int head, tail,cap;

	SPIN_LOCK(q)
	head = q->head;
	tail = q->tail;
	cap = q->cap;
	SPIN_UNLOCK(q)
	
	if (head <= tail) {
		return tail - head;
	}
	return tail + cap - head;
}

// 消息队列过载
int
uboss_mq_overload(struct message_queue *q) {
	if (q->overload) {
		int overload = q->overload;
		q->overload = 0;
		return overload;
	} 
	return 0;
}

// 从服务消息队列中弹出消息
int
uboss_mq_pop(struct message_queue *q, struct uboss_message *message) {
	int ret = 1;
	SPIN_LOCK(q)

	// 如果队列头不等于队列尾
	if (q->head != q->tail) {
		*message = q->queue[q->head++];
		ret = 0;
		int head = q->head;
		int tail = q->tail;
		int cap = q->cap;

		if (head >= cap) {
			q->head = head = 0;
		}
		int length = tail - head;
		if (length < 0) {
			length += cap;
		}
		while (length > q->overload_threshold) {
			q->overload = length;
			q->overload_threshold *= 2;
		}
	} else {
		// reset overload_threshold when queue is empty
		q->overload_threshold = MQ_OVERLOAD;
	}

	if (ret) {
		q->in_global = 0;
	}
	
	SPIN_UNLOCK(q)

	return ret;
}

static void
expand_queue(struct message_queue *q) {
	struct uboss_message *new_queue = uboss_malloc(sizeof(struct uboss_message) * q->cap * 2);
	int i;
	for (i=0;i<q->cap;i++) {
		new_queue[i] = q->queue[(q->head + i) % q->cap];
	}
	q->head = 0;
	q->tail = q->cap;
	q->cap *= 2;
	
	uboss_free(q->queue);
	q->queue = new_queue;
}

// 将消息压入服务的消息队列中
void 
uboss_mq_push(struct message_queue *q, struct uboss_message *message) {
	assert(message);
	SPIN_LOCK(q) // 锁住

	q->queue[q->tail] = *message;
	if (++ q->tail >= q->cap) {
		q->tail = 0;
	}

	if (q->head == q->tail) {
		expand_queue(q);
	}

	if (q->in_global == 0) {
		q->in_global = MQ_IN_GLOBAL;
		uboss_globalmq_push(q);
	}
	
	SPIN_UNLOCK(q) // 解锁
}

// 初始化服务的消息队列
void 
uboss_mq_init() {
	struct global_queue *q = uboss_malloc(sizeof(*q)); // 声明一个全局队列的结构
	memset(q,0,sizeof(*q)); // 清空结构
	SPIN_INIT(q); // 初始化锁
	Q=q; // 将结构的指针赋给全局变量
}

void 
uboss_mq_mark_release(struct message_queue *q) {
	SPIN_LOCK(q)
	assert(q->release == 0);
	q->release = 1;
	if (q->in_global != MQ_IN_GLOBAL) {
		uboss_globalmq_push(q);
	}
	SPIN_UNLOCK(q)
}

static void
_drop_queue(struct message_queue *q, message_drop drop_func, void *ud) {
	struct uboss_message msg;
	while(!uboss_mq_pop(q, &msg)) {
		drop_func(&msg, ud);
	}
	_release(q);
}

void 
uboss_mq_release(struct message_queue *q, message_drop drop_func, void *ud) {
	SPIN_LOCK(q)
	
	if (q->release) {
		SPIN_UNLOCK(q)
		_drop_queue(q, drop_func, ud);
	} else {
		uboss_globalmq_push(q);
		SPIN_UNLOCK(q)
	}
}
