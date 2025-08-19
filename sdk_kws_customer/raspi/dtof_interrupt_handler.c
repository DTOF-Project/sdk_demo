#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <gpiod.h>
#include <sys/time.h>
#include <openssl/buffer.h>  // Add this for BUF_MEM
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <string.h>

#include "dtof_interrupt_handler.h"
#include "gpio_init.h"
#include "signal_manager.h"
#include "inc/dtof_common.h"
#include "inc/dtof_api.h"
#include "customer/dtof_customer.h"
#include "platform_user_config.h"
#include <cJSON.h>

#include "dtof_frame_control.h"


// 修改数据处理函数
// 添加全局配置变量
static dtof_reg_read_mask_t reg_read_mask = {
    .read_reg_fifo_data = true,
    .read_fifo_data = true,
    .read_main_hist = false,
    .read_ref_hist = false,
    .read_raw_data = false
};

void dtof_set_reg_read_mask(const dtof_reg_read_mask_t *mask) {
    memcpy(&reg_read_mask, mask, sizeof(dtof_reg_read_mask_t));
}

static void process_dtof_data(bool is_new_frame) {
    static uint32_t frame_index = 0;
    dtof_raw_data_t raw_data;

    struct timeval tv;
    gettimeofday(&tv, NULL);
    raw_data.milliseconds = tv.tv_sec * 1000 + tv.tv_usec / 1000;  // 秒转毫秒 + 微秒转毫秒

    raw_data.frame_index = frame_index++;
    raw_data.is_new_frame = is_new_frame;

    if (dtof_get_fifo(&raw_data.result_info) != DTOF_RET_SUCCESS) {
        printf("FIFO读取失败\n");
        return;
    }

    // 根据配置选择性读取数据
    bool read_success = true;
    if (reg_read_mask.read_reg_fifo_data) {
        read_success &= (dtof_reg_burst_read(0x50, raw_data.reg_fifo, 10) == DTOF_RET_SUCCESS);
    }
    if (reg_read_mask.read_fifo_data) {
        read_success &= (dtof_reg_burst_read(0x50, raw_data.fifo_buf, 64) == DTOF_RET_SUCCESS);
    }
    if (reg_read_mask.read_main_hist) {
        read_success &= (dtof_reg_burst_read(0x60, raw_data.main_hist, 64) == DTOF_RET_SUCCESS);
    }
    if (reg_read_mask.read_ref_hist) {
        read_success &= (dtof_reg_burst_read(0x70, raw_data.ref_hist, 64) == DTOF_RET_SUCCESS);
    }
    if (reg_read_mask.read_raw_data) {
        read_success &= (dtof_reg_burst_read(0x80, raw_data.raw_data, 64) == DTOF_RET_SUCCESS);
    }

    if (!read_success) {
        printf("寄存器读取失败\n");
        return;
    }

    if (dtof_queue_push(&raw_data) != 0) {
        printf("数据入队列失败\n");
    }

    // for(int i = 0; i < 10;i++)
    // {
    //     printf("%d, ", *((uint16_t*)&raw_data.result_info + i));
    // }
    // printf("\n");
}

// 添加新的函数用于等待和读取中断事件
// 添加定时器相关定义
#define TIMER_INTERVAL_MS 33  // 约30Hz的采样率

// 修改等待和读取中断事件函数
static int wait_and_read_interrupt_event(struct gpiod_line_event *event) {
    struct timespec ts = {0, TIMER_INTERVAL_MS * 1000000};  // 转换为纳秒
    int ret;

    // 等待中断事件或定时器超时
    ret = gpiod_line_event_wait(intr_line, &ts);
    if (ret < 0) {
        perror("等待 DTOF 中断事件出错");
        return -1;
    } else if (ret == 0) {
        return 0;  // 无数据
    }

    // 读取并清除事件状态
    if (gpiod_line_event_read(intr_line, event) < 0) {
        perror("读取 DTOF 中断事件出错");
        return -2;
    }

    return 1;  // 中断触发
}

int polling_mode = 0;
static void* dtof_interrupt_thread(void* arg) {
    struct gpiod_line_event event;
    int ret;
    int is_new_frame = 0;
    // 设置线程为最高优先级
    struct sched_param param;
    param.sched_priority = 99;
    if (pthread_setschedparam(pthread_self(), SCHED_FIFO, &param) != 0) {
        perror("无法设置 DTOF 中断处理线程优先级");
        return (void*)DTOF_HANDLER_PRIORITY_SET_FAILED;
    }

    printf("DTOF 中断处理线程启动，优先级 99...\n");

    while (signal_manager_should_continue()) {

        if (polling_mode == 1) {
            struct timeval tv;
            tv.tv_sec = 0;
            tv.tv_usec = 33333;  // 33.33ms
            select(0, NULL, NULL, NULL, &tv);  // 等待超时
        } else {
            ret = wait_and_read_interrupt_event(&event);
            if (ret < 0) {
                break;      // 发生错误
            } else if (ret == 0) {
                continue;   // 无数据，继续等待
            }
            dtof_set_interrupt_flag(DTOF_TRUE);
            is_new_frame = 1;
        }

        // 处理 DTOF 数据
        if(get_dtof_mode_type()){
            process_dtof_data(is_new_frame);
        }

        is_new_frame = 0;
        if (polling_mode == 1){
            // runonce
            is_new_frame = 1;
        }
        // // 根据触发源打印不同信息
        // if (ret == 1) {
        //     printf("DTOF 中断触发处理完成！时间戳: %ld.%ld\n",
        //            event.ts.tv_sec, event.ts.tv_nsec);
        // } else {
        //     printf("DTOF 定时采样处理完成！\n");
        // }
    }
    printf(" interrupt thread close\n");
    return NULL;
}

static pthread_t dtof_thread;  // 将线程句柄移到模块内部

int dtof_interrupt_handler_start(void) {
    if (pthread_create(&dtof_thread, NULL, dtof_interrupt_thread, NULL) != 0) {
        perror("DTOF 中断处理线程创建失败");
        return DTOF_HANDLER_THREAD_CREATE_FAILED;
    }
    return DTOF_HANDLER_OK;
}

void dtof_interrupt_handler_wait(void) {
    pthread_join(dtof_thread, NULL);
}