#include "uboss.h"
#include "uboss_server.h"
#include "uboss_imp.h"
#include "uboss_mq.h"
#include "uboss_handle.h"
#include "uboss_module.h"
#include "uboss_timer.h"
#include "uboss_monitor.h"
#include "uboss_socket.h"
#include "uboss_daemon.h"
#include "uboss_harbor.h"

#include <pthread.h>
#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct monitor {
	int count;
	struct uboss_monitor ** m;
	pthread_cond_t cond;
	pthread_mutex_t mutex;
	int sleep;
	int quit;
};

struct worker_parm {
	struct monitor *m;
	int id;
	int weight;
};

#define CHECK_ABORT if (uboss_context_total()==0) break;

static void
create_thread(pthread_t *thread, void *(*start_routine) (void *), void *arg) {
	if (pthread_create(thread,NULL, start_routine, arg)) {
		fprintf(stderr, "Create thread failed");
		exit(1);
	}
}

static void
wakeup(struct monitor *m, int busy) {
	if (m->sleep >= m->count - busy) {
		// signal sleep worker, "spurious wakeup" is harmless
		pthread_cond_signal(&m->cond);
	}
}

static void *
thread_socket(void *p) {
	struct monitor * m = p;
	uboss_initthread(THREAD_SOCKET);
	for (;;) {
		int r = uboss_socket_poll();
		if (r==0)
			break;
		if (r<0) {
			CHECK_ABORT
			continue;
		}
		wakeup(m,0);
	}
	return NULL;
}

static void
free_monitor(struct monitor *m) {
	int i;
	int n = m->count;
	for (i=0;i<n;i++) {
		uboss_monitor_delete(m->m[i]);
	}
	pthread_mutex_destroy(&m->mutex);
	pthread_cond_destroy(&m->cond);
	uboss_free(m->m);
	uboss_free(m);
}

static void *
thread_monitor(void *p) {
	struct monitor * m = p;
	int i;
	int n = m->count;
	uboss_initthread(THREAD_MONITOR);
	for (;;) {
		CHECK_ABORT
		for (i=0;i<n;i++) {
			uboss_monitor_check(m->m[i]);
		}
		for (i=0;i<5;i++) {
			CHECK_ABORT
			sleep(1);
		}
	}

	return NULL;
}

static void *
thread_timer(void *p) {
	struct monitor * m = p;
	uboss_initthread(THREAD_TIMER);
	for (;;) {
		uboss_updatetime();
		CHECK_ABORT
		wakeup(m,m->count-1);
		usleep(2500);
	}
	// wakeup socket thread
	uboss_socket_exit();
	// wakeup all worker thread
	pthread_mutex_lock(&m->mutex);
	m->quit = 1;
	pthread_cond_broadcast(&m->cond);
	pthread_mutex_unlock(&m->mutex);
	return NULL;
}

static void *
thread_worker(void *p) {
	struct worker_parm *wp = p;
	int id = wp->id;
	int weight = wp->weight;
	struct monitor *m = wp->m;
	struct uboss_monitor *sm = m->m[id];
	uboss_initthread(THREAD_WORKER);
	struct message_queue * q = NULL;
	while (!m->quit) {
		q = uboss_context_message_dispatch(sm, q, weight);
		if (q == NULL) {
			if (pthread_mutex_lock(&m->mutex) == 0) {
				++ m->sleep;
				// "spurious wakeup" is harmless,
				// because uboss_context_message_dispatch() can be call at any time.
				if (!m->quit)
					pthread_cond_wait(&m->cond, &m->mutex);
				-- m->sleep;
				if (pthread_mutex_unlock(&m->mutex)) {
					fprintf(stderr, "unlock mutex error");
					exit(1);
				}
			}
		}
	}
	return NULL;
}

static void
start(int thread) {
	pthread_t pid[thread+3];

	struct monitor *m = uboss_malloc(sizeof(*m));
	memset(m, 0, sizeof(*m));
	m->count = thread;
	m->sleep = 0;

	m->m = uboss_malloc(thread * sizeof(struct uboss_monitor *));
	int i;
	for (i=0;i<thread;i++) {
		m->m[i] = uboss_monitor_new();
	}
	if (pthread_mutex_init(&m->mutex, NULL)) {
		fprintf(stderr, "Init mutex error");
		exit(1);
	}
	if (pthread_cond_init(&m->cond, NULL)) {
		fprintf(stderr, "Init cond error");
		exit(1);
	}

	create_thread(&pid[0], thread_monitor, m);
	create_thread(&pid[1], thread_timer, m);
	create_thread(&pid[2], thread_socket, m);

	static int weight[] = { 
		-1, -1, -1, -1, 0, 0, 0, 0,
		1, 1, 1, 1, 1, 1, 1, 1, 
		2, 2, 2, 2, 2, 2, 2, 2, 
		3, 3, 3, 3, 3, 3, 3, 3, };
	struct worker_parm wp[thread];
	for (i=0;i<thread;i++) {
		wp[i].m = m;
		wp[i].id = i;
		if (i < sizeof(weight)/sizeof(weight[0])) {
			wp[i].weight= weight[i];
		} else {
			wp[i].weight = 0;
		}
		create_thread(&pid[i+3], thread_worker, &wp[i]);
	}

	for (i=0;i<thread+3;i++) {
		pthread_join(pid[i], NULL); 
	}

	free_monitor(m);
}

static void
bootstrap(struct uboss_context * logger, const char * cmdline) {
	int sz = strlen(cmdline);
	char name[sz+1];
	char args[sz+1];
	sscanf(cmdline, "%s %s", name, args);
	struct uboss_context *ctx = uboss_context_new(name, args);
	if (ctx == NULL) {
		uboss_error(NULL, "Bootstrap error : %s\n", cmdline);
		uboss_context_dispatchall(logger);
		exit(1);
	}
}

void 
uboss_start(struct uboss_config * config) {
	if (config->daemon) {
		if (daemon_init(config->daemon)) {
			exit(1);
		}
	}
	uboss_harbor_init(config->harbor);
	uboss_handle_init(config->harbor);
	uboss_mq_init();
	uboss_module_init(config->module_path);
	uboss_timer_init();
	uboss_socket_init();

	struct uboss_context *ctx = uboss_context_new(config->logservice, config->logger);
	if (ctx == NULL) {
		fprintf(stderr, "Can't launch %s service\n", config->logservice);
		exit(1);
	}

	bootstrap(ctx, config->bootstrap);

	start(config->thread);

	// harbor_exit may call socket send, so it should exit before socket_free
	uboss_harbor_exit();
	uboss_socket_free();
	if (config->daemon) {
		daemon_exit(config->daemon);
	}
}
