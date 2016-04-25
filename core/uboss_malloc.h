#ifndef uboss_malloc_h
#define uboss_malloc_h

#include <stddef.h>

#define uboss_malloc malloc
#define uboss_calloc calloc
#define uboss_realloc realloc
#define uboss_free free

void * uboss_malloc(size_t sz);
void * uboss_calloc(size_t nmemb,size_t size);
void * uboss_realloc(void *ptr, size_t size);
void uboss_free(void *ptr);
char * uboss_strdup(const char *str);
void * uboss_lalloc(void *ud, void *ptr, size_t osize, size_t nsize);	// use for lua

#endif
