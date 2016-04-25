#include "uboss.h"

#include "uboss_monitor.h"
#include "uboss_server.h"
#include "uboss.h"
#include "atomic.h"

#include <stdlib.h>
#include <string.h>

struct uboss_monitor {
	int version;
	int check_version;
	uint32_t source;
	uint32_t destination;
};

struct uboss_monitor * 
uboss_monitor_new() {
	struct uboss_monitor * ret = uboss_malloc(sizeof(*ret));
	memset(ret, 0, sizeof(*ret));
	return ret;
}

void 
uboss_monitor_delete(struct uboss_monitor *sm) {
	uboss_free(sm);
}

void 
uboss_monitor_trigger(struct uboss_monitor *sm, uint32_t source, uint32_t destination) {
	sm->source = source;
	sm->destination = destination;
	ATOM_INC(&sm->version);
}

void 
uboss_monitor_check(struct uboss_monitor *sm) {
	if (sm->version == sm->check_version) {
		if (sm->destination) {
			uboss_context_endless(sm->destination);
			uboss_error(NULL, "A message from [ :%08x ] to [ :%08x ] maybe in an endless loop (version = %d)", sm->source , sm->destination, sm->version);
		}
	} else {
		sm->check_version = sm->version;
	}
}
