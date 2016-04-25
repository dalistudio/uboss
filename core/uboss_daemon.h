#ifndef UBOSS_DAEMON_H
#define UBOSS_DAEMON_H

int daemon_init(const char *pidfile);
int daemon_exit(const char *pidfile);

#endif
