#ifndef UBOSS_MODULE_H
#define UBOSS_MODULE_H

struct uboss_context;

typedef void * (*uboss_dl_create)(void);
typedef int (*uboss_dl_init)(void * inst, struct uboss_context *, const char * parm);
typedef void (*uboss_dl_release)(void * inst);
typedef void (*uboss_dl_signal)(void * inst, int signal);

struct uboss_module {
	const char * name;
	void * module;
	uboss_dl_create create;
	uboss_dl_init init;
	uboss_dl_release release;
	uboss_dl_signal signal;
};

void uboss_module_insert(struct uboss_module *mod);
struct uboss_module * uboss_module_query(const char * name);
void * uboss_module_instance_create(struct uboss_module *);
int uboss_module_instance_init(struct uboss_module *, void * inst, struct uboss_context *ctx, const char * parm);
void uboss_module_instance_release(struct uboss_module *, void *inst);
void uboss_module_instance_signal(struct uboss_module *, void *inst, int signal);

void uboss_module_init(const char *path);

#endif
