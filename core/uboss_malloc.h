/*
** Copyright (c) 2014-2016 uboss.org All rights Reserved.
** uBoss - A lightweight micro service framework
**
** uBoss Malloc
**
** Dali Wang<dali@uboss.org>
** See Copyright Notice in uboss.h
*/

#ifndef uboss_malloc_h
#define uboss_malloc_h

#include <stddef.h>
#include <lua.h>

#define uboss_malloc malloc
#define uboss_calloc calloc
#define uboss_realloc realloc
#define uboss_free free

extern size_t malloc_used_memory(void);
extern size_t malloc_memory_block(void);
extern void   memory_info_dump(void);
extern size_t mallctl_int64(const char* name, size_t* newval);
extern int    mallctl_opt(const char* name, int* newval);
extern void   dump_c_mem(void);
extern int    dump_mem_lua(lua_State *L);
extern size_t malloc_current_memory(void);

void * uboss_malloc(size_t sz);
void * uboss_calloc(size_t nmemb,size_t size);
void * uboss_realloc(void *ptr, size_t size);
void uboss_free(void *ptr);
char * uboss_strdup(const char *str);
void * uboss_lalloc(void *ud, void *ptr, size_t osize, size_t nsize);	// use for lua

#endif
