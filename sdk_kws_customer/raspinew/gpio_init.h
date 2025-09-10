#ifndef GPIO_INIT_H
#define GPIO_INIT_H

#include <gpiod.h>

// 全局变量声明
extern struct gpiod_line *intr_line;
extern struct gpiod_chip *chip;

// 函数声明
int rpi_gpio_init(void);
void rpi_gpio_cleanup(void);

/**
 * @brief 使能或禁用GPIO中断
 * @param enable true:使能中断 false:禁用中断
 * @return 0:成功 <0:失败
 */
int rpi_gpio_interrupt_enable(bool enable);
#ifdef GPIO_DEBUG
void rpi_gpio_debug_up_down(int status);
#endif

#endif /* GPIO_INIT_H */