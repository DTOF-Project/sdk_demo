/*
 * main.c
 *
 *  Created on: 2024/9/11
 *      Author: liuzihao
 */
#include "main.h"
#include "inc/dtof_api.h"
#include "inc/dtof_log.h"
#include "inc/dtof_driver.h"
#include "inc/dtof_endian.h"
#include "inc/dtof_calibration_ft.h"
#include "inc/dtof_global_config.h"
#include "inc/dev/dtof_dev_api.h"
#include "base/inc/mos_platform.h"
#include "data_base/sensor_database.h"
#include "customer/dtof_customer.h"

#include "dev/dtof_hal.h"

#include "user/device/ds_sal.h"
#include "user/device/ds_dev.h"
#include "user/device/device.h"

#include "application/inc/soc_version.h"
#include "application/inc/app_cmd.h"

extern int stm32_uart_write(int uart_id, void *buf, int nbyte);
extern int stm32_uart_read(int uart_id, void *buf, int nbyte);

static dtof_uint16_t is_to_sky_flag = 1;

// 这里要求一定是输入的是 int16 的数据
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
/**************************************************************/

#define SPECIAL_BYPASS_VALUE 7
#define SPECIAL_LOOP_VALUE 136
#define DTOF_ENABLE_DEBUG_MODE 1
#define DTOF_DISABLE_DEBUG_MODE 0
#define DTOF_WFI_STATUS_FLAG_ADDR 0x6e
#define DTOF_WFI_STATUS_FLAG 0xab
DTOF_RET dtof_set_debug_mode(dtof_int32_t debug_mode)
{
    if (debug_mode == DTOF_ENABLE_DEBUG_MODE)
    {
        DTOF_CHECK_RET(dtof_io_interaction(DTOF_CMD_WRITE_REG_ADDR, SPECIAL_BYPASS_VALUE), "enable debug mode failed\n");
    }
    else if (debug_mode == DTOF_DISABLE_DEBUG_MODE)
    {
        DTOF_CHECK_RET(dtof_io_interaction(DTOF_CMD_WRITE_REG_ADDR, SPECIAL_LOOP_VALUE), "disable debug mode failed\n");
    }
    else
    {
        DTOF_LOG_ERR("invalid debug mode\n");
        return DTOF_RET_FAILED;
    }

    return DTOF_RET_SUCCESS;
}

DTOF_RET dtof_read_innermcu_intr_control_flag(dtof_uint16_t *intr_control_flag)
{
    DTOF_CHECK_RET(dtof_reg_burst_read(DTOF_WFI_STATUS_FLAG_ADDR, intr_control_flag, 1), "read reg 0x6e failed\n");

    return DTOF_RET_SUCCESS;
}

DTOF_RET dtof_debug_mode_bypass(dtof_chip_type_t chip_type)
{
    DTOF_RET ret = DTOF_RET_SUCCESS;
    if (chip_type == DTOF_CHIP_TYPE_A05)
    {
        // a05使用inner mcu中断里的bypass, 偶发会导致inner mcu crash, 需要特殊处理, 使用外部的bypass, 且bypass前关闭timer, dsp, eyesafe中断, 进入wfi, 唤醒后
        dtof_uint16_t intr_control_flag;
        dtof_uint16_t bypassvalue = 0x17b9;
        DTOF_CHECK_RET(dtof_read_innermcu_intr_control_flag(&intr_control_flag), "read inner mcu status failed\n");
        if (intr_control_flag != DTOF_WFI_STATUS_FLAG)
        {
            DTOF_LOG_ERR("intr_control_flag is 0x%x\n", intr_control_flag);
            ret = DTOF_RET_FAILED;
        }

        DTOF_CHECK_RET(dtof_reg_burst_write(DTOF_IO_CTRL_REG_ADDR, &bypassvalue, 1), "write bypass value failed\n");
    }
    else if (chip_type == DTOF_CHIP_TYPE_L3)
    {
        DTOF_CHECK_RET(dtof_set_mcu_status(DTOF_MCU_STATE_SLEEP_DIRECT), "set mcu sleep failed\n");
    }
    else
    {
        DTOF_LOG_ERR("unknown chip type\n");
        ret = DTOF_RET_FAILED;
    }
    return ret;
}

/**
 * @brief
 * @return int
 */
int main(void)
{
    DTOF_RET ret;
    dtof_uint16_t chip_id;
    dtof_device_t *dev = ds_device_get();
    dtof_uint16_t buffer[DTOF_SINGLE_MAIN_HISTGRAM_LEN + 64];
    dtof_distance_result_t distance_result;
    dtof_uint16_t reg80;
    dtof_bool_t is_new_flag;
    dtof_bool_t first_new_flag = DTOF_TRUE; // polling模式下, 不取第一帧, debug模式下需要特殊处理, 因为新的bypass交互流程
    dtof_bool_t is_init = DTOF_FALSE;
    dtof_bool_t debug_flag = DTOF_FALSE;

    DTOF_CHECK_WARN(platform_init(), "platform init failed\n");

    DTOF_CHECK_WARN(dtof_peripheral_device_init(), "dtof_peripheral_device_init failed\n");

    dtof_find_chip_config(DTOF_L3_CHIPID, 0,0);

    // DTOF_CHECK_WARN(dtof_sensor_init(), "dtof sensor init failed\n");

    // save chip uuid
    // usleep(10000);
    // DTOF_CHECK_WARN(dtof_get_uuid(dev->chip_uuid, DTOF_UUID_LENGTH), "get uuid failed\n");

    // // save chip type
    // DTOF_CHECK_WARN(ds_get_chip_type(dtof_get_chip_config()->chip_id, &dev->chip_type), "get chip type failed\n");

    uint8_t byte;
    dtof_bool_t frame_cnt_flag = DTOF_FALSE;
    int32_t frame_cnt = 0;

    dtof_set_interrupt_flag(DTOF_FALSE);

    while (1)
    {
        is_new_flag = DTOF_FALSE;
        int len = dev->device_uart_driver->read(0, &byte, 1);

        if (len == 1)
        {
            parse_cmd_process(byte);
        }
    }

    return 0;
}
