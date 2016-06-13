/*
 * 监视器
 *
 * 用于监视工作线程是否已死，并关闭服务
*/
#ifndef UBOSS_MONITOR_H
#define UBOSS_MONITOR_H

#include <stdint.h>

struct uboss_monitor;

struct uboss_monitor * uboss_monitor_new();
void uboss_monitor_delete(struct uboss_monitor *um);
void uboss_monitor_trigger(struct uboss_monitor *um, uint32_t source, uint32_t destination);
void uboss_monitor_check(struct uboss_monitor *um);

#endif
