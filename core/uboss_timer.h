/*
 * 定时器模块
 *
 * 提供定时器事件链表，管理定时器，并到期调用回调函数
*/
#ifndef UBOSS_TIMER_H
#define UBOSS_TIMER_H

#include <stdint.h>

int uboss_timeout(uint32_t handle, int time, int session);
void uboss_updatetime(void);
uint32_t uboss_starttime(void);

void uboss_timer_init(void);

#endif
