/*
 * platform_user_config.h
 *
 *  Created on: 2024/9/11
 *      Author: liuzihao
 */

#ifndef PLATFORM_RASPBERRY_PI_INCLUDE_USER_CONFIG_H
#define PLATFORM_RASPBERRY_PI_INCLUDE_USER_CONFIG_H
#include "sdk/inc/dtof_base_type.h"
#include "sdk/inc/dtof_log.h"
// GPIO 配置
#define GPIO_CHIP_NAME      "gpiochip0"  // GPIO 控制器名称
#define GPIO_INTR_LINE     14            // 中断引脚号，使用 GPIO14
#define GPIO_RESET_LINE    15            // 复位引脚号，使用 GPIO15

// I2C 配置
#define I2C_BUS           "/dev/i2c-1"   // I2C 总线设备文件
#define DEVICE_ADDR       0x41           // I2C 从设备地址

// GPIO 操作相关配置
#define GPIO_RESET_DELAY_US    100       // 复位延时（微秒）
#define GPIO_SETUP_DELAY_US    750       // 设置延时（微秒）

#endif  // PLATFORM_RASPBERRY_PI_INCLUDE_USER_CONFIG_H
