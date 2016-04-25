#include "uboss.h"
#include "uboss_harbor.h"
#include "uboss_server.h"
#include "uboss_mq.h"
#include "uboss_handle.h"

#include <string.h>
#include <stdio.h>
#include <assert.h>

static struct uboss_context * REMOTE = 0;
static unsigned int HARBOR = ~0;

void 
uboss_harbor_send(struct remote_message *rmsg, uint32_t source, int session) {
	int type = rmsg->sz >> MESSAGE_TYPE_SHIFT;
	rmsg->sz &= MESSAGE_TYPE_MASK;
	assert(type != PTYPE_SYSTEM && type != PTYPE_HARBOR && REMOTE);
	uboss_context_send(REMOTE, rmsg, sizeof(*rmsg) , source, type , session);
}

int 
uboss_harbor_message_isremote(uint32_t handle) {
	assert(HARBOR != ~0);
	int h = (handle & ~HANDLE_MASK);
	return h != HARBOR && h !=0;
}

void
uboss_harbor_init(int harbor) {
	HARBOR = (unsigned int)harbor << HANDLE_REMOTE_SHIFT;
}

void
uboss_harbor_start(void *ctx) {
	// the HARBOR must be reserved to ensure the pointer is valid.
	// It will be released at last by calling uboss_harbor_exit
	uboss_context_reserve(ctx);
	REMOTE = ctx;
}

void
uboss_harbor_exit() {
	struct uboss_context * ctx = REMOTE;
	REMOTE= NULL;
	if (ctx) {
		uboss_context_release(ctx);
	}
}
