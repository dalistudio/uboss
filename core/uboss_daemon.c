/*
** Copyright (c) 2014-2017 uboss.org All rights Reserved.
** uBoss - A Lightweight MicroService Framework
**
** uBoss Daemon
**
** Dali Wang<dali@uboss.org>
** See Copyright Notice in uboss.h
*/

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/file.h>
#include <signal.h>
#include <errno.h>
#include <stdlib.h>

#include "uboss_daemon.h"

// 检查 PID 文件
static int
check_pid(const char *pidfile) {
	int pid = 0;
	FILE *f = fopen(pidfile,"r"); // 以只读方式打开 PID 文件
	if (f == NULL)
		return 0;
	int n = fscanf(f,"%d", &pid); // 读取 PID 的值
	fclose(f); // 关闭文件

	// 如果 PID 不为1 或 PID为0 或 PID为当前进程号，则返回0
	if (n !=1 || pid == 0 || pid == getpid()) {
		return 0;
	}

	// 向进程发送0信号，测试一下进程是否运行
	if (kill(pid, 0) && errno == ESRCH)
		return 0;

	return pid;
}

// 写 PID 文件
static int 
write_pid(const char *pidfile) {
	FILE *f;
	int pid = 0;
	int fd = open(pidfile, O_RDWR|O_CREAT, 0644); // 打开 PID 文件

	// 不能创建文件
	if (fd == -1) {
		fprintf(stderr, "Can't create %s.\n", pidfile);
		return 0;
	}

	// 打开文件流
	f = fdopen(fd, "r+");
	if (f == NULL) {
		fprintf(stderr, "Can't open %s.\n", pidfile);
		return 0;
	}

	if (flock(fd, LOCK_EX|LOCK_NB) == -1) {
		int n = fscanf(f, "%d", &pid);
		fclose(f); // 关闭文件流
		if (n != 1) {
			fprintf(stderr, "Can't lock and read pidfile.\n");
		} else {
			fprintf(stderr, "Can't lock pidfile, lock is held by pid %d.\n", pid);
		}
		return 0;
	}
	
	// 获得当前进程号
	pid = getpid();
	if (!fprintf(f,"%d\n", pid)) {
		fprintf(stderr, "Can't write pid.\n");
		close(fd); // 关闭文件
		return 0;
	}
	fflush(f); // 输出流

	return pid;
}

// 初始化守护
int
daemon_init(const char *pidfile) {
	int pid = check_pid(pidfile); // 检查 PID

	if (pid) {
		fprintf(stderr, "uBoss is already running, pid = %d.\n", pid);
		return 1;
	}

#ifdef __APPLE__
	fprintf(stderr, "'daemon' is deprecated: first deprecated in OS X 10.5 , use launchd instead.\n");
#else
	if (daemon(1,0)) { // 守护进程，输出到窗口
		fprintf(stderr, "Can't daemonize.\n");
		return 1;
	}
#endif

	// 写 PID
	pid = write_pid(pidfile);
	if (pid == 0) {
		return 1;
	}

	return 0;
}

// 退出守护
int 
daemon_exit(const char *pidfile) {
	return unlink(pidfile); // 删除 PID 文件
}
