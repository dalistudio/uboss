#ifndef UBOSS_IMP_H
#define UBOSS_IMP_H

struct uboss_config {
	int thread;
	int harbor;
	const char * daemon;
	const char * module_path;
	const char * bootstrap;
	const char * logger;
	const char * logservice;
};

#define THREAD_WORKER 0
#define THREAD_MAIN 1
#define THREAD_SOCKET 2
#define THREAD_TIMER 3
#define THREAD_MONITOR 4

void uboss_start(struct uboss_config * config);

#endif
