#ifndef uboss_hashid_h
#define uboss_hashid_h

#include <assert.h>
#include <stdlib.h>
#include <string.h>

// 散列ID 的节点
struct hashid_node {
	int id; // ID 值
	struct hashid_node *next; // 下一节点
};

// 散列ID 的结构
struct hashid {
	int hashmod; // 散列的模块
	int cap;
	int count; // 总数
	struct hashid_node *id;
	struct hashid_node **hash;
};

// 初始化 散列ID
static void
hashid_init(struct hashid *hi, int max) {
	int i;
	int hashcap;
	hashcap = 16;
	while (hashcap < max) {
		hashcap *= 2;
	}
	hi->hashmod = hashcap - 1;
	hi->cap = max;
	hi->count = 0;
	hi->id = uboss_malloc(max * sizeof(struct hashid_node)); // 分配内存空间
	for (i=0;i<max;i++) {
		hi->id[i].id = -1;
		hi->id[i].next = NULL;
	}
	hi->hash = uboss_malloc(hashcap * sizeof(struct hashid_node *)); // 分配内存空间
	memset(hi->hash, 0, hashcap * sizeof(struct hashid_node *)); // 清空内存
}

// 清理 散列ID
static void
hashid_clear(struct hashid *hi) {
	uboss_free(hi->id);
	uboss_free(hi->hash);
	hi->id = NULL;
	hi->hash = NULL;
	hi->hashmod = 1;
	hi->cap = 0;
	hi->count = 0;
}

// 查找 散列ID
static int
hashid_lookup(struct hashid *hi, int id) {
	int h = id & hi->hashmod;
	struct hashid_node * c = hi->hash[h];
	while(c) {
		if (c->id == id)
			return c - hi->id;
		c = c->next;
	}
	return -1;
}

// 移除 散列ID
static int
hashid_remove(struct hashid *hi, int id) {
	int h = id & hi->hashmod;
	struct hashid_node * c = hi->hash[h];
	if (c == NULL)
		return -1;
	if (c->id == id) {
		hi->hash[h] = c->next;
		goto _clear;
	}
	while(c->next) {
		if (c->next->id == id) {
			struct hashid_node * temp = c->next;
			c->next = temp->next;
			c = temp;
			goto _clear;
		}
		c = c->next;
	}
	return -1;
_clear:
	c->id = -1;
	c->next = NULL;
	--hi->count;
	return c - hi->id;
}

// 插入 散列ID
static int
hashid_insert(struct hashid * hi, int id) {
	struct hashid_node *c = NULL;
	int i;
	for (i=0;i<hi->cap;i++) {
		int index = (i+id) % hi->cap;
		if (hi->id[index].id == -1) {
			c = &hi->id[index];
			break;
		}
	}
	assert(c);
	++hi->count;
	c->id = id;
	assert(c->next == NULL);
	int h = id & hi->hashmod;
	if (hi->hash[h]) {
		c->next = hi->hash[h];
	}
	hi->hash[h] = c;
	
	return c - hi->id;
}

// 散列ID 满
static inline int
hashid_full(struct hashid *hi) {
	return hi->count == hi->cap;
}

#endif