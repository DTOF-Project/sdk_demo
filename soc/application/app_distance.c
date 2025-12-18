#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "inc/dtof_base_type.h"
#include "inc/dtof_driver.h"
#include "inc/dtof_api.h"
#include "inc/dtof_log.h"
#include "inc/dev/dtof_hal.h"
#include "inc/dev/dtof_dev_api.h"

#include "application/inc/app_distance.h"

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

DTOF_RET dtof_enter_debug_mode(void)
{
    dtof_uint16_t ram_fsm_state;

    // 判断ram是否跑到wfi
    DTOF_CHECK_RET(dtof_get_ram_fsm_state(&ram_fsm_state), "get ram fsm state failed\n");
    if ((ram_fsm_state != RAM_STATE_RUNNING))
    {
        DTOF_LOG_ERR("ram not in running or idle state, state = %d\n", ram_fsm_state);
        return DTOF_RET_FAILED;
    }

    DTOF_CHECK_RET(dtof_bypass_inner_mcu_external(), "bypass inner mcu failed\n");
    return DTOF_RET_SUCCESS;
}

DTOF_RET dtof_trigger_next_frame(void)
{
    // 唤醒mcu
    DTOF_CHECK_RET(dtof_wakeup_inner_mcu_external(), "wakeup inner mcu failed\n");
    // 给05中断退出wfi
    DTOF_CHECK_RET(dtof_io_interaction(DTOF_CMD_NONE, DTOF_INVALID_CMD_VALUE), "send io cmd fail\n");

    return DTOF_RET_SUCCESS;
}

void dtof_output_distance_result(dtof_distance_result_t distance_result)
{
    dtof_uint16_t buffer[DTOF_SINGLE_MAIN_HISTGRAM_LEN + 64];
    switch (g_distance_mode)
    {
        case DISTANCE_NORMAL_MODE:
        {
            printf("%d, %d, %d, %d, %.6f, 1\n",
                            distance_result.frame_id, distance_result.first_target, distance_result.first_intensity, distance_result.main_nflash, distance_result.ambient);
            break;
        }
        case DISTANCE_DEBUG_MODE:
        {
            dtof_enter_debug_mode();

        #define TOTAL_REG_NUM 255
            dtof_histgram_io_read(DTOF_SINGLE_MAIN_HISTGRAM_OFFSET, buffer, DTOF_SINGLE_MAIN_HISTGRAM_LEN);
            dump_hist_log(buffer, DTOF_SINGLE_MAIN_HISTGRAM_LEN);
            dtof_histgram_io_read(DTOF_SINGLE_REF_HISTGRAM_OFFSET, buffer, DTOF_SINGLE_REF_HISTGRAM_LEN);
            dump_hist_log(buffer, DTOF_SINGLE_REF_HISTGRAM_LEN);
            dtof_dsp_fifo_read(0, buffer, DTOF_SINGLE_FIFO_LEN);
            dump_hist_log(buffer, DTOF_SINGLE_FIFO_LEN);
            dtof_reg_burst_read(0x00, buffer, TOTAL_REG_NUM);
            dump_hist_log(buffer, TOTAL_REG_NUM);

            dtof_trigger_next_frame();

            printf("%d, %d, %d, %d, %.6f, 1\n",
                            distance_result.frame_id, distance_result.first_target, distance_result.first_intensity, distance_result.main_nflash, distance_result.ambient);
            break;
        }
        case DISTANCE_TEST_MODE:
        {
            static int test_frame_count = 0;
            dtof_enter_debug_mode();

        #define TOTAL_REG_NUM 255
            dtof_histgram_io_read(DTOF_SINGLE_MAIN_HISTGRAM_OFFSET, buffer, DTOF_SINGLE_MAIN_HISTGRAM_LEN);
            dump_hist_log(buffer, DTOF_SINGLE_MAIN_HISTGRAM_LEN);
            dtof_histgram_io_read(DTOF_SINGLE_REF_HISTGRAM_OFFSET, buffer, DTOF_SINGLE_REF_HISTGRAM_LEN);
            dump_hist_log(buffer, DTOF_SINGLE_REF_HISTGRAM_LEN);
            dtof_dsp_fifo_read(0, buffer, DTOF_SINGLE_FIFO_LEN);
            dump_hist_log(buffer, DTOF_SINGLE_FIFO_LEN);
            dtof_reg_burst_read(0x00, buffer, TOTAL_REG_NUM);
            dump_hist_log(buffer, TOTAL_REG_NUM);

            if (++test_frame_count >= DISTANCE_TEST_MODE_FRAME_NUM)
            {
                DTOF_CHECK_WARN(dtof_stop_distance_measure(), "dtof stop distance mode failed\n");
                test_frame_count = 0;
            }
            else
            {
                dtof_trigger_next_frame();
            }

            printf("%d, %d, %d, %d, %.6f, 1\n",
                            distance_result.frame_id, distance_result.first_target, distance_result.first_intensity, distance_result.main_nflash, distance_result.ambient);

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
