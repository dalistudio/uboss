/*
** Copyright (c) 2014-2016 uboss.org All rights Reserved.
** uBoss - A Lightweight MicroService Framework
**
** uBoss License
**
** Dali Wang<dali@uboss.org>
** See Copyright Notice in uboss.h
*/

#include "uboss.h"

#include "uboss_license.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 许可证验证
int
uboss_license_verify() {
	fprintf(stdout, "verify License ...\n");
	return 0;
}

// 许可证初始化
void 
uboss_license_init(void) {
	fprintf(stdout, "This is License\n");
}

