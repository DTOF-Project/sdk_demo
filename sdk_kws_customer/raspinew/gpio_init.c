#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <gpiod.h>

#include "gpio_init.h"
#include "platform_user_config.h"

// 全局变量定义
struct gpiod_line *intr_line = NULL;
struct gpiod_chip *chip = NULL;

DTOF_RET rpi_gpio_init(void) {
    struct gpiod_line *line;
    int ret;

    // 打开 GPIO 芯片
    chip = gpiod_chip_open_by_name(GPIO_CHIP_NAME);
    if (!chip) {
        perror("无法打开 GPIO 芯片");
        return DTOF_FAIL;
    }

    // 获取复位引脚
    line = gpiod_chip_get_line(chip, GPIO_RESET_LINE);
    if (!line) {
        perror("无法获取复位引脚");
        gpiod_chip_close(chip);
        return DTOF_FAIL;
    }

     // 配置复位引脚为输出，并初始化为低电平
     ret = gpiod_line_request_output(line, "reset_output", 0);  // 0表示低电平
     if (ret < 0) {
         perror("无法配置复位引脚为输出");
         gpiod_chip_close(chip);
         return DTOF_FAIL;
     }

    // 等待一段时间，模拟操作
    usleep(100);

    // 设置复位引脚为高电平
    ret = gpiod_line_set_value(line, 1);  // 1表示高电平
    if (ret < 0) {
        perror("无法设置复位引脚为高电平");
        gpiod_line_release(line);  // 释放 line 资源
        gpiod_chip_close(chip);
        return DTOF_FAIL;
    }

    usleep(750);

    // 获取中断引脚
    intr_line = gpiod_chip_get_line(chip, GPIO_INTR_LINE);
    if (!intr_line) {
        perror("无法获取中断引脚");
        gpiod_line_release(line);  // 释放 line 资源
        gpiod_chip_close(chip);
        return DTOF_FAIL;
    }

    // 配置引脚为带上拉电阻的输入，检测下降沿
    ret = gpiod_line_request_falling_edge_events_flags(intr_line, "interrupt_handler",
                                                     GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_UP);
    if (ret < 0) {
        perror("无法请求 GPIO 事件");
        gpiod_line_release(line);  // 释放 line 资源
        gpiod_chip_close(chip);
        return DTOF_FAIL;
    }

    // 释放复位引脚，因为我们只需要它进行初始化
    gpiod_line_release(line);

    return DTOF_SUCCESS;
}

int rpi_gpio_interrupt_enable(bool enable) {
    if (!intr_line) {
        return -1;
    }

    if (enable) {
        // 重新配置为下降沿触发中断
        return gpiod_line_request_falling_edge_events_flags(intr_line,
            "interrupt_handler", GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_UP);
    } else {
        // 临时释放中断线，但保持 intr_line 指针
        gpiod_line_release(intr_line);
        // 重新配置为输入，但不触发中断
        return gpiod_line_request_input_flags(intr_line,
            "interrupt_disabled", GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_UP);
    }
}

// 修改清理函数，增加状态检查
void rpi_gpio_cleanup(void) {
    if (intr_line) {
        // 确保在清理前禁用中断
        rpi_gpio_interrupt_enable(false);
        gpiod_line_release(intr_line);
        intr_line = NULL;
    }

    if (chip) {
        gpiod_chip_close(chip);
        chip = NULL;
    }
}