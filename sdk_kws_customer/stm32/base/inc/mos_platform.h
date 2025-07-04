/*
 * mos_platform.h
 *
 *  Created on: 2024/9/11
 *      Author: liuzihao
 */
#ifndef DRIVERS_BASE_INCLUDE_MOS_PLATFORM_H_
#define DRIVERS_BASE_INCLUDE_MOS_PLATFORM_H_

#include <stdlib.h>

#include "mos_def.h"

#ifdef __cplusplus
extern "C" {
#endif

// 平台初始化
DTOF_RET platform_init();

// AP端延时
void usleep(int micro_seconds);
// 获得时间戳
uint64_t generated_us(void);

#ifdef __cplusplus
}
#endif

#endif  // DRIVERS_BASE_INCLUDE_MOS_PLATFORM_H_
