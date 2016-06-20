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

// 设置 公钥数据：椭圆曲线 + 公钥
int
_set_public_key() {
	return 0;
}

// 获得 CPUID 的数据，并取SHA1散列前4字节返回
int
_get_cpu_id() {
	// 厂商：AuthenticAMD
	// 型号：178BFBFF00610F01
	// 商标：AMD A10-5800K APU with Radeon(tm) HD Graphics  
	// Salt：盐值，即预留的一个字符串，防止散列破解
	// SHA1(厂商+型号+商标+Salt)
	return 0;
}

// 获得 网卡MAC 的数据，并取SHA1散列前4字节返回
int
_get_nic_mac() {
	// 名称：Realtek PCIe GBE Family Controller
	// 型号：PCI\VEN_10EC&DEV_8168
	// MAC ：B8-97-5A-27-30-42
	// Salt：盐值，即预留的一个字符串，防止散列破解
	// SHA1(名称+型号+MAC+Salt)
	return 0;
}

// 获取本机硬件散列
int
_get_hardware_hash() {
	// 获取 CPUID 相关数据
	int cpuid = _get_cpu_id();

	// 获取 网卡MAC 相关数据
	int nicmac = _get_nic_mac();

	// SHA1(CPUID + MAC + Salt)

	// 返回值，用于计算许可证
	return 0;
}

// 读 0磁道
int
_get_track_sectors(int sector, char *data, int len) {
	return 0;
}

// 写 0磁道
int
_set_track_sectors(int sector, char *data, int len) {
	return 0;
}

// 激活系统
int
uboss_lincense_activator(char * license) {
	return 0;
}

// 功能验证
int
uboss_license_function(int function) {
	return 0;
}
 
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

