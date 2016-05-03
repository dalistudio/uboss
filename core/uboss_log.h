/*
 * 日志模块
 *
 * 如果开启，可记录日志到文件或网络中
*/
#ifndef uboss_log_h
#define uboss_log_h

#include "uboss_env.h"
#include "uboss.h"

#include <stdio.h>
#include <stdint.h>

FILE * uboss_log_open(struct uboss_context * ctx, uint32_t handle);
void uboss_log_close(struct uboss_context * ctx, FILE *f, uint32_t handle);
void uboss_log_output(FILE *f, uint32_t source, int type, int session, void * buffer, size_t sz);

#endif
