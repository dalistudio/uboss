#ifndef UBOSS_H
#define UBOSS_H

#include "uboss_malloc.h"

#include <stddef.h>
#include <stdint.h>

#define PTYPE_TEXT 0
#define PTYPE_RESPONSE 1
#define PTYPE_MULTICAST 2
#define PTYPE_CLIENT 3
#define PTYPE_SYSTEM 4
#define PTYPE_HARBOR 5
#define PTYPE_SOCKET 6
// read lualib/uboss.lua examples/simplemonitor.lua
#define PTYPE_ERROR 7	
// read lualib/uboss.lua lualib/mqueue.lua lualib/snax.lua
#define PTYPE_RESERVED_QUEUE 8
#define PTYPE_RESERVED_DEBUG 9
#define PTYPE_RESERVED_LUA 10
#define PTYPE_RESERVED_SNAX 11

#define PTYPE_TAG_DONTCOPY 0x10000
#define PTYPE_TAG_ALLOCSESSION 0x20000

struct uboss_context;

void uboss_error(struct uboss_context * context, const char *msg, ...);
const char * uboss_command(struct uboss_context * context, const char * cmd , const char * parm);
uint32_t uboss_queryname(struct uboss_context * context, const char * name);
int uboss_send(struct uboss_context * context, uint32_t source, uint32_t destination , int type, int session, void * msg, size_t sz);
int uboss_sendname(struct uboss_context * context, uint32_t source, const char * destination , int type, int session, void * msg, size_t sz);

int uboss_isremote(struct uboss_context *, uint32_t handle, int * harbor);

typedef int (*uboss_cb)(struct uboss_context * context, void *ud, int type, int session, uint32_t source , const void * msg, size_t sz);
void uboss_callback(struct uboss_context * context, void *ud, uboss_cb cb);

uint32_t uboss_current_handle(void);
uint64_t uboss_now(void);
void uboss_debug_memory(const char *info);	// for debug use, output current service memory to stderr

#endif
