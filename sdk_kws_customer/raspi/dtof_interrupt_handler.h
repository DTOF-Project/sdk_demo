#ifndef DTOF_INTERRUPT_HANDLER_H
#define DTOF_INTERRUPT_HANDLER_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>
#include "inc/dtof_api.h"         // 移动到inc/dtof_common.h之前
#include "inc/dtof_common.h"

// DTOF 中断处理线程的状态码
enum dtof_handler_status {
    DTOF_HANDLER_OK = 0,
    DTOF_HANDLER_THREAD_CREATE_FAILED = -1,
    DTOF_HANDLER_PRIORITY_SET_FAILED = -2
};

// 启动 DTOF 中断处理线程
int dtof_interrupt_handler_start(void);

// 等待 DTOF 中断处理线程结束
void dtof_interrupt_handler_wait(void);

// 添加数据结构定义
typedef struct {
    uint64_t milliseconds;
    uint32_t frame_index;
    dtof_distance_result_t result_info;
    uint16_t reg_fifo[10];
    uint16_t fifo_buf[64];
    uint16_t main_hist[64];
    uint16_t ref_hist[64];
    uint16_t raw_data[512];
    bool is_new_frame;
} dtof_raw_data_t;

// 添加寄存器读取配置位
typedef struct {
    bool read_reg_fifo_data;// 是否读取REG FIFO数据
    bool read_fifo_data;    // 是否读取DSP FIFO数据
    bool read_main_hist;    // 是否读取主直方图
    bool read_ref_hist;     // 是否读取参考直方图
    bool read_raw_data;     // 是否读取原始数据
} dtof_reg_read_mask_t;

// 声明配置函数
void dtof_set_reg_read_mask(const dtof_reg_read_mask_t *mask);

// 声明队列相关函数
void dtof_queue_init(void);
void dtof_queue_deinit(void);
int dtof_queue_push(dtof_raw_data_t *data);
int dtof_queue_pop(dtof_raw_data_t *data);

#endif // DTOF_INTERRUPT_HANDLER_H