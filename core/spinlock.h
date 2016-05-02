#ifndef UBOSS_SPINLOCK_H
#define UBOSS_SPINLOCK_H

#define SPIN_INIT(q) spinlock_init(&(q)->lock);
#define SPIN_LOCK(q) spinlock_lock(&(q)->lock);
#define SPIN_UNLOCK(q) spinlock_unlock(&(q)->lock);
#define SPIN_DESTROY(q) spinlock_destroy(&(q)->lock);

#ifndef USE_PTHREAD_LOCK

// 自旋锁的结构
struct spinlock {
	int lock; // 锁
};

// 初始化 自旋锁
static inline void
spinlock_init(struct spinlock *lock) {
	lock->lock = 0; // 设置锁为0
}

// 锁住
static inline void
spinlock_lock(struct spinlock *lock) {
	while (__sync_lock_test_and_set(&lock->lock,1)) {}
}

// 尝试锁住
static inline int
spinlock_trylock(struct spinlock *lock) {
	return __sync_lock_test_and_set(&lock->lock,1) == 0;
}

// 解锁
static inline void
spinlock_unlock(struct spinlock *lock) {
	__sync_lock_release(&lock->lock);
}

// 销毁锁
static inline void
spinlock_destroy(struct spinlock *lock) {
	(void) lock;
}

#else

// 使用线程锁
#include <pthread.h>

// we use mutex instead of spinlock for some reason
// you can also replace to pthread_spinlock

// 自旋锁的结构
struct spinlock {
	pthread_mutex_t lock;
};

// 初始化 自旋锁
static inline void
spinlock_init(struct spinlock *lock) {
	pthread_mutex_init(&lock->lock, NULL);
}

// 锁住
static inline void
spinlock_lock(struct spinlock *lock) {
	pthread_mutex_lock(&lock->lock);
}

// 尝试锁住
static inline int
spinlock_trylock(struct spinlock *lock) {
	return pthread_mutex_trylock(&lock->lock) == 0;
}

// 解锁
static inline void
spinlock_unlock(struct spinlock *lock) {
	pthread_mutex_unlock(&lock->lock);
}

// 销毁锁
static inline void
spinlock_destroy(struct spinlock *lock) {
	pthread_mutex_destroy(&lock->lock);
}

#endif

#endif
