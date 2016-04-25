#ifndef UBOSS_SERVER_H
#define UBOSS_SERVER_H

#include <stdint.h>
#include <stdlib.h>

struct uboss_context;
struct uboss_message;
struct uboss_monitor;

struct uboss_context * uboss_context_new(const char * name, const char * parm);
void uboss_context_grab(struct uboss_context *);
void uboss_context_reserve(struct uboss_context *ctx);
struct uboss_context * uboss_context_release(struct uboss_context *);
uint32_t uboss_context_handle(struct uboss_context *);
int uboss_context_push(uint32_t handle, struct uboss_message *message);
void uboss_context_send(struct uboss_context * context, void * msg, size_t sz, uint32_t source, int type, int session);
int uboss_context_newsession(struct uboss_context *);
struct message_queue * uboss_context_message_dispatch(struct uboss_monitor *, struct message_queue *, int weight);	// return next queue
int uboss_context_total();
void uboss_context_dispatchall(struct uboss_context * context);	// for uboss_error output before exit

void uboss_context_endless(uint32_t handle);	// for monitor

void uboss_globalinit(void);
void uboss_globalexit(void);
void uboss_initthread(int m);

#endif
