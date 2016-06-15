/*
** Copyright (c) 2014-2016 uboss.org All rights Reserved.
** uBoss - A Lightweight MicroService Framework
**
** uBoss Socket
**
** Dali Wang<dali@uboss.org>
** See Copyright Notice in uboss.h
*/

#ifndef uboss_socket_h
#define uboss_socket_h

struct uboss_context;

#define UBOSS_SOCKET_TYPE_DATA 1
#define UBOSS_SOCKET_TYPE_CONNECT 2
#define UBOSS_SOCKET_TYPE_CLOSE 3
#define UBOSS_SOCKET_TYPE_ACCEPT 4
#define UBOSS_SOCKET_TYPE_ERROR 5
#define UBOSS_SOCKET_TYPE_UDP 6
#define UBOSS_SOCKET_TYPE_WARNING 7

struct uboss_socket_message {
	int type;
	int id;
	int ud;
	char * buffer;
};

void uboss_socket_init();
void uboss_socket_exit();
void uboss_socket_free();
int uboss_socket_poll();

int uboss_socket_send(struct uboss_context *ctx, int id, void *buffer, int sz);
void uboss_socket_send_lowpriority(struct uboss_context *ctx, int id, void *buffer, int sz);
int uboss_socket_listen(struct uboss_context *ctx, const char *host, int port, int backlog);
int uboss_socket_connect(struct uboss_context *ctx, const char *host, int port);
int uboss_socket_bind(struct uboss_context *ctx, int fd);
void uboss_socket_close(struct uboss_context *ctx, int id);
void uboss_socket_shutdown(struct uboss_context *ctx, int id);
void uboss_socket_start(struct uboss_context *ctx, int id);
void uboss_socket_nodelay(struct uboss_context *ctx, int id);

int uboss_socket_udp(struct uboss_context *ctx, const char * addr, int port);
int uboss_socket_udp_connect(struct uboss_context *ctx, int id, const char * addr, int port);
int uboss_socket_udp_send(struct uboss_context *ctx, int id, const char * address, const void *buffer, int sz);
const char * uboss_socket_udp_address(struct uboss_socket_message *, int *addrsz);

#endif
