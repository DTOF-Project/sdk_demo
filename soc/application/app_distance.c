#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "inc/dtof_base_type.h"
#include "inc/dtof_driver.h"
#include "inc/dtof_api.h"
#include "inc/dtof_log.h"

#include "application/inc/app_distance.h"

// TODO: 不要放在stm32的文件夹下
#include "dev/dtof_hal.h"

int g_distance_mode = DISTANCE_UNKNOWN_MODE;

void app_set_distance_mode(int mode)
{
    g_distance_mode = mode;
}

int app_get_distance_mode(void)
{
    return g_distance_mode;
}

extern int stm32_uart_write(int uart_id, void *buf, int nbyte);
void dump_hist_log(dtof_uint16_t *hist_p, dtof_uint16_t len)
{
#define OUT_PUT_MAX_BUFFER (517)
    char output_str[OUT_PUT_MAX_BUFFER]; // 确保缓冲区足够大
    char *temp_start = output_str;
    int total_used_len = 0;

// 每个数据的单元大小是 5 = 4bytes hex + ，
#define PRINT_UNIT_SIZE 5
    for (int i = 0; i < len; i++)
    {
        int pos;
        if (hist_p[i] <= 0xff)
        {
            pos = snprintf(temp_start, PRINT_UNIT_SIZE, "%x,", hist_p[i]);
        }
        else if (hist_p[i] <= 0xfff)
        {
            pos = snprintf(temp_start, PRINT_UNIT_SIZE + 1, "%03x,", hist_p[i]);
        }
        else if (hist_p[i] <= 0xffff)
        {
            pos = snprintf(temp_start, PRINT_UNIT_SIZE + 2, "%04x,", hist_p[i]);
        }
        total_used_len = total_used_len + pos;
        temp_start = temp_start + pos;

        if (total_used_len >= (OUT_PUT_MAX_BUFFER - PRINT_UNIT_SIZE))
        {
            // sendout the value
            stm32_uart_write(0, output_str, total_used_len);
            // reset the value
            temp_start = output_str;
            total_used_len = 0;
        }
    }

    if (total_used_len != 0)
    {
        stm32_uart_write(0, output_str, total_used_len);
    }
    stm32_uart_write(0, "\n", 1);
}

void dtof_determine_new_frame(dtof_bool_t *is_new_flag, dtof_distance_result_t *distance_result)
{
    if (g_distance_mode != DISTANCE_UNKNOWN_MODE)
    {
    #ifdef DTOF_INTERRUPT_MODE
        if (dtof_get_interrupt_flag() == DTOF_TRUE)
        {
            *is_new_flag = DTOF_TRUE;
            dtof_get_distance_result(distance_result);
            dtof_set_interrupt_flag(DTOF_FALSE);
        }
    #elif defined(DTOF_POLLING_MODE)
        dtof_get_distance_result_polling(distance_result, is_new_flag);
    #endif
    }

    return;
}

DTOF_RET dtof_enable_distance_debug_mode(void)
{
#define SPECIAL_BYPASS_VALUE 7
    DTOF_CHECK_RET(dtof_io_interaction(DTOF_CMD_WRITE_REG_ADDR, SPECIAL_BYPASS_VALUE), "enable debug mode failed\n");
    return DTOF_RET_SUCCESS;
}

DTOF_RET dtof_read_innermcu_intr_control_flag(dtof_uint16_t *intr_control_flag)
{
#define DTOF_WFI_STATUS_FLAG_ADDR 0x6e
    DTOF_CHECK_RET(dtof_reg_burst_read(DTOF_WFI_STATUS_FLAG_ADDR, intr_control_flag, 1), "read reg 0x6e failed\n");
    return DTOF_RET_SUCCESS;
}

DTOF_RET dtof_bypass_distance_debug_mode(void)
{
#define DTOF_WFI_STATUS_FLAG 0xab
#define DTOF_WAIT_WFI_STATUS_DELAY 10
#define DTOF_WAIT_WFI_STATUS_TRY_COUNT 4
    dtof_int32_t try_count = 0;
    dtof_uint16_t intr_control_flag;
    dtof_uint16_t bypassvalue = 0x17b9;

    // TODO: 第二次d / e的时候这里会出问题, 所以加了循环判断, 为什么？
    do {
        DTOF_CHECK_RET(dtof_read_innermcu_intr_control_flag(&intr_control_flag), "read inner mcu status failed\n");
        dtof_sleep_ms(DTOF_WAIT_WFI_STATUS_DELAY);
    } while((intr_control_flag != DTOF_WFI_STATUS_FLAG) && (try_count++ < DTOF_WAIT_WFI_STATUS_TRY_COUNT));

    if (try_count >= DTOF_WAIT_WFI_STATUS_TRY_COUNT)
    {
        DTOF_LOG_ERR("intr_control_flag is not 0xab, is 0x%x\n", intr_control_flag);
        return DTOF_RET_FAILED;
    }

    DTOF_CHECK_RET(dtof_reg_burst_write(DTOF_IO_CTRL_REG_ADDR, &bypassvalue, 1), "write bypass value failed\n");
    return DTOF_RET_SUCCESS;
}

void dtof_output_distance_result(dtof_distance_result_t distance_result)
{
    dtof_uint16_t buffer[DTOF_SINGLE_MAIN_HISTGRAM_LEN + 64];
    switch (g_distance_mode)
    {
        case DISTANCE_NORMAL_MODE:
        {
            printf("%d, %d, %d, %d, %.6f, %d\n",
                            distance_result.frame_id, distance_result.first_target, distance_result.first_intensity, distance_result.main_nflash, distance_result.ambient, distance_result.is_legal_frame);
            break;
        }
        case DISTANCE_DEBUG_MODE:
        {
            dtof_bypass_distance_debug_mode();

        #define TOTAL_REG_NUM 255
            dtof_histgram_io_read(DTOF_SINGLE_MAIN_HISTGRAM_OFFSET, buffer, DTOF_SINGLE_MAIN_HISTGRAM_LEN);
            dump_hist_log(buffer, DTOF_SINGLE_MAIN_HISTGRAM_LEN);
            dtof_histgram_io_read(DTOF_SINGLE_REF_HISTGRAM_OFFSET, buffer, DTOF_SINGLE_REF_HISTGRAM_LEN);
            dump_hist_log(buffer, DTOF_SINGLE_REF_HISTGRAM_LEN);
            dtof_dsp_fifo_read(0, buffer, DTOF_SINGLE_FIFO_LEN);
            dump_hist_log(buffer, DTOF_SINGLE_FIFO_LEN);
            dtof_reg_burst_read(0x00, buffer, TOTAL_REG_NUM);
            dump_hist_log(buffer, TOTAL_REG_NUM);

            dtof_set_mcu_status(DTOF_MCU_STATE_WAKEUP);

            dtof_io_interaction(0x30, 0x01);

            dtof_enable_distance_debug_mode();

            printf("%d, %d, %d, %d, %.6f, %d\n",
                            distance_result.frame_id, distance_result.first_target, distance_result.first_intensity, distance_result.main_nflash, distance_result.ambient, distance_result.is_legal_frame);
        }
        case DISTANCE_TEST_MODE:
        {
            static int test_frame_count = 0;
            dtof_bypass_distance_debug_mode();

            if (++test_frame_count >= DISTANCE_TEST_MODE_FRAME_NUM)
            {
                uint16_t stop_flag = DTOF_STOP_DISATNCE_MODE;
                app_set_distance_mode(DISTANCE_UNKNOWN_MODE);
                dtof_reg_burst_write(DTOF_FRAME_CONTROL_REG, &stop_flag, 1); // TODO: 使用running的写会唤醒mcu
                // DTOF_CHECK_WARN(dtof_stop_distance_measure(), "dtof stop distance mode failed\n");
                test_frame_count = 0;
            }

        #define TOTAL_REG_NUM 255
            dtof_histgram_io_read(DTOF_SINGLE_MAIN_HISTGRAM_OFFSET, buffer, DTOF_SINGLE_MAIN_HISTGRAM_LEN);
            dump_hist_log(buffer, DTOF_SINGLE_MAIN_HISTGRAM_LEN);
            dtof_histgram_io_read(DTOF_SINGLE_REF_HISTGRAM_OFFSET, buffer, DTOF_SINGLE_REF_HISTGRAM_LEN);
            dump_hist_log(buffer, DTOF_SINGLE_REF_HISTGRAM_LEN);
            dtof_dsp_fifo_read(0, buffer, DTOF_SINGLE_FIFO_LEN);
            dump_hist_log(buffer, DTOF_SINGLE_FIFO_LEN);
            dtof_reg_burst_read(0x00, buffer, TOTAL_REG_NUM);
            dump_hist_log(buffer, TOTAL_REG_NUM);

            dtof_set_mcu_status(DTOF_MCU_STATE_WAKEUP);

            dtof_io_interaction(0x30, 0x01);

            dtof_enable_distance_debug_mode();

            printf("%d, %d, %d, %d, %.6f, %d\n",
                            distance_result.frame_id, distance_result.first_target, distance_result.first_intensity, distance_result.main_nflash, distance_result.ambient, distance_result.is_legal_frame);
            break;
        }
        default:
            break;
    }
    return;
}

void app_distance_process(void)
{
    dtof_distance_result_t distance_result;
    dtof_bool_t is_new_flag = DTOF_FALSE;

    dtof_determine_new_frame(&is_new_flag, &distance_result);

    if (is_new_flag == DTOF_TRUE)
    {
        dtof_output_distance_result(distance_result);
    }

    return;
}
