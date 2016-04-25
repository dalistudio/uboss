#ifndef UBOSS_TIMER_H
#define UBOSS_TIMER_H

#include <stdint.h>

int uboss_timeout(uint32_t handle, int time, int session);
void uboss_updatetime(void);
uint32_t uboss_starttime(void);

void uboss_timer_init(void);

#endif
