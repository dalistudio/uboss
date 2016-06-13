/*
** Copyright (c) 2014-2016 uboss.org All rights Reserved.
** uBoss - A lightweight micro service framework
**
** uBoss Handle
**
** Dali Wang<dali@uboss.org>
** See Copyright Notice in uboss.h
*/

#ifndef UBOSS_CONTEXT_HANDLE_H
#define UBOSS_CONTEXT_HANDLE_H

#include <stdint.h>

// reserve high 8 bits for remote id
#define HANDLE_MASK 0xffffff
#define HANDLE_REMOTE_SHIFT 24

struct uboss_context;

uint32_t uboss_handle_register(struct uboss_context *);
int uboss_handle_retire(uint32_t handle);
struct uboss_context * uboss_handle_grab(uint32_t handle);
void uboss_handle_retireall();

uint32_t uboss_handle_findname(const char * name);
const char * uboss_handle_namehandle(uint32_t handle, const char *name);

void uboss_handle_init(int harbor);

#endif
