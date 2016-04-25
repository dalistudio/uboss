#ifndef UBOSS_MONITOR_H
#define UBOSS_MONITOR_H

#include <stdint.h>

struct uboss_monitor;

struct uboss_monitor * uboss_monitor_new();
void uboss_monitor_delete(struct uboss_monitor *);
void uboss_monitor_trigger(struct uboss_monitor *, uint32_t source, uint32_t destination);
void uboss_monitor_check(struct uboss_monitor *);

#endif
