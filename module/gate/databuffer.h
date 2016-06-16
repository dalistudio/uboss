/*
** Copyright (c) 2014-2016 uboss.org All rights Reserved.
** uBoss - A Lightweight MicroService Framework
**
** uBoss Gate Module
**
** Dali Wang<dali@uboss.org>
** See Copyright Notice in uboss.h
*/

#ifndef uboss_databuffer_h
#define uboss_databuffer_h

#include <stdlib.h>
#include <string.h>
#include <assert.h>

// 消息池的长度
#define MESSAGEPOOL 1023

// 消息的结构
struct message {
	char * buffer; // 缓冲区
	int size; // 大小
	struct message * next; // 下一缓冲区
};

// 数据缓冲区的结构
struct databuffer {
	int header; // 头部
	int offset; // 偏移
	int size; // 大小
	struct message * head; // 头部指针
	struct message * tail; // 尾部指针
};

// 消息池链表的结构
struct messagepool_list {
	struct messagepool_list *next; // 下一消息池的指针
	struct message pool[MESSAGEPOOL]; // 消息池数组
};

// 消息池
struct messagepool {
	struct messagepool_list * pool; // 消息池链表的指针
	struct message * freelist; // 释放消息链表的指针
};

// use memset init struct 

// 释放消息池
static void 
messagepool_free(struct messagepool *pool) {
	struct messagepool_list *p = pool->pool;
	while(p) {
		struct messagepool_list *tmp = p;
		p=p->next;
		uboss_free(tmp);
	}
	pool->pool = NULL;
	pool->freelist = NULL;
}

// 返回消息
static inline void
_return_message(struct databuffer *db, struct messagepool *mp) {
	struct message *m = db->head;
	if (m->next == NULL) {
		assert(db->tail == m);
		db->head = db->tail = NULL;
	} else {
		db->head = m->next;
	}
	uboss_free(m->buffer);
	m->buffer = NULL;
	m->size = 0;
	m->next = mp->freelist;
	mp->freelist = m;
}

// 从数据缓冲区读取数据
static void
databuffer_read(struct databuffer *db, struct messagepool *mp, void * buffer, int sz) {
	assert(db->size >= sz);
	db->size -= sz;
	for (;;) {
		struct message *current = db->head;
		int bsz = current->size - db->offset;
		if (bsz > sz) {
			memcpy(buffer, current->buffer + db->offset, sz);
			db->offset += sz;
			return;
		}
		if (bsz == sz) {
			memcpy(buffer, current->buffer + db->offset, sz);
			db->offset = 0;
			_return_message(db, mp);
			return;
		} else {
			memcpy(buffer, current->buffer + db->offset, bsz);
			_return_message(db, mp);
			db->offset = 0;
			buffer+=bsz;
			sz-=bsz;
		}
	}
}

// 将数据压入数据缓冲区
static void
databuffer_push(struct databuffer *db, struct messagepool *mp, void *data, int sz) {
	struct message * m;
	if (mp->freelist) {
		m = mp->freelist;
		mp->freelist = m->next;
	} else {
		struct messagepool_list * mpl = uboss_malloc(sizeof(*mpl));
		struct message * temp = mpl->pool;
		int i;
		for (i=1;i<MESSAGEPOOL;i++) {
			temp[i].buffer = NULL;
			temp[i].size = 0;
			temp[i].next = &temp[i+1];
		}
		temp[MESSAGEPOOL-1].next = NULL;
		mpl->next = mp->pool;
		mp->pool = mpl;
		m = &temp[0];
		mp->freelist = &temp[1];
	}
	m->buffer = data;
	m->size = sz;
	m->next = NULL;
	db->size += sz;
	if (db->head == NULL) {
		assert(db->tail == NULL);
		db->head = db->tail = m;
	} else {
		db->tail->next = m;
		db->tail = m;
	}
}

// 从数据缓冲区中读取头数据
static int
databuffer_readheader(struct databuffer *db, struct messagepool *mp, int header_size) {
	if (db->header == 0) {
		// parser header (2 or 4)
		if (db->size < header_size) {
			return -1;
		}
		uint8_t plen[4];
		databuffer_read(db,mp,(char *)plen,header_size);
		// big-endian
		if (header_size == 2) {
			db->header = plen[0] << 8 | plen[1];
		} else {
			db->header = plen[0] << 24 | plen[1] << 16 | plen[2] << 8 | plen[3];
		}
	}
	if (db->size < db->header)
		return -1;
	return db->header;
}

// 重置数据缓冲区
static inline void
databuffer_reset(struct databuffer *db) {
	db->header = 0;
}

// 清理数据缓冲区
static void
databuffer_clear(struct databuffer *db, struct messagepool *mp) {
	while (db->head) {
		_return_message(db,mp);
	}
	memset(db, 0, sizeof(*db)); // 清空内存
}

#endif
