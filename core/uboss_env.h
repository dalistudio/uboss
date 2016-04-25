#ifndef UBOSS_ENV_H
#define UBOSS_ENV_H

const char * uboss_getenv(const char *key);
void uboss_setenv(const char *key, const char *value);

void uboss_env_init();

#endif
