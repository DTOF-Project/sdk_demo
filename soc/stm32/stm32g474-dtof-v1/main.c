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
#include "base/inc/mos_platform.h"
#include "data_base/sensor_database.h"
#include "customer/dtof_customer.h"

#include "dev/dtof_hal.h"

#include "user/device/ds_sal.h"
#include "user/device/ds_dev.h"
#include "user/device/device.h"

#include "application/inc/soc_version.h"
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
    dtof_bool_t is_new_flag;
    dtof_bool_t is_init = DTOF_FALSE;
    dtof_bool_t debug_flag = DTOF_FALSE;

    DTOF_CHECK_WARN(platform_init(), "platform init failed\n");

    DTOF_CHECK_WARN(dtof_peripheral_device_init(), "dtof_peripheral_device_init failed\n");

    DTOF_CHECK_WARN(dtof_sensor_init(), "dtof sensor init failed\n");

    // save chip uuid
    DTOF_CHECK_WARN(dtof_get_uuid(dev->chip_uuid, DTOF_UUID_LENGTH), "get uuid failed\n");

    // save chip type
    DTOF_CHECK_WARN(ds_get_chip_type(dtof_get_chip_config()->chip_id, &dev->chip_type), "get chip type failed\n");

    uint8_t byte;
    char uart_buf[32] = {0};
    uint8_t uart_index = 0;

    dtof_bool_t frame_cnt_flag = DTOF_FALSE;
    int32_t frame_cnt = 0;

    dtof_set_interrupt_flag(DTOF_FALSE);

    while (1)
    {
        int len = stm32_uart_read(0, &byte, 1);

        if (len == 1)
        {
            if (byte == '\n' || byte == '\r')
            {                                // 一条命令结束
                uart_buf[uart_index] = '\0'; // 添加字符串结束符

                // 命令解析
                if (strcmp(uart_buf, "s") == 0)
                {
                    DTOF_CHECK_WARN(dtof_sensor_init(), "dtof sensor init failed\n");
                    dtof_start_distance_measure();
                    is_init = DTOF_TRUE;
                    debug_flag = DTOF_FALSE;
                }
                else if (strcmp(uart_buf, "d") == 0)
                {
                    DTOF_CHECK_WARN(dtof_sensor_init(), "dtof sensor init failed\n");
                    dtof_start_distance_measure();
                    if (dev->chip_type == DTOF_CHIP_TYPE_A05)
                    {
                        DTOF_CHECK_WARN(dtof_io_interaction(DTOF_CMD_WRITE_REG_ADDR, SPECIAL_BYPASS_VALUE), "enable debug mode failed\n");
                    }
                    is_init = DTOF_TRUE;
                    debug_flag = DTOF_TRUE;
                }
                else if (strcmp(uart_buf, "e") == 0)
                {
                    DTOF_CHECK_WARN(dtof_sensor_init(), "dtof sensor init failed\n");
                    dtof_start_distance_measure();
                    if (dev->chip_type == DTOF_CHIP_TYPE_A05)
                    {
                        DTOF_CHECK_WARN(dtof_io_interaction(DTOF_CMD_WRITE_REG_ADDR, SPECIAL_BYPASS_VALUE), "enable debug mode failed\n");
                    }
                    frame_cnt_flag = DTOF_TRUE;
                    is_init = DTOF_TRUE;
                    debug_flag = DTOF_TRUE;
                }
                else if (strcmp(uart_buf, "t") == 0)
                {
                STOP_DISTANCE_MEASURE:
                    dtof_stop_distance_measure();
                    if (debug_flag == DTOF_TRUE)
                    {
                        if (dev->chip_type == DTOF_CHIP_TYPE_A05)
                        {
                            DTOF_CHECK_WARN(dtof_io_interaction(DTOF_CMD_WRITE_REG_ADDR, SPECIAL_BYPASS_VALUE), "disable debug mode failed\n");
                        }
                    }
                }
                else if (strncmp(uart_buf, "r,", 2) == 0)
                {
                    int reg_addr = atoi(&uart_buf[2]);
                    uint16_t reg_value;
                    dtof_read_reg_running(reg_addr, &reg_value);
                    printf("reg read 0x%x: 0x%4x\n", reg_addr, reg_value);
                }
                else if (strncmp(uart_buf, "rb,", 3) == 0)
                {
                    DTOF_CHECK_RET(dtof_set_mcu_status_ram(DTOF_MCU_STATE_SLEEP_DIRECT), "set mcu sleep failed\n");
                    int reg_addr, reg_num;
                    dtof_uint16_t reg_max[255];
                    if (sscanf(uart_buf, "rb,%d,%d", &reg_addr, &reg_num) == 2)
                    {
                        dtof_reg_burst_read(reg_addr, reg_max, reg_num);
                        printf("burst reg read 0x%04x:\n", reg_addr);
                        for (int i = 0; i < reg_num; i++)
                        {
                            printf("%d, ", reg_max[i]);
                        }
                        printf("\n");
                    }
                    DTOF_CHECK_RET(dtof_set_mcu_status_ram(DTOF_MCU_STATE_WAKEUP), "wakeup mcu failed\n");
                }
                else if (strncmp(uart_buf, "w,", 2) == 0)
                {
                    int reg_addr, reg_value;
                    if (sscanf(uart_buf, "w,%d,%d", &reg_addr, &reg_value) == 2)
                    {
                        if (reg_addr == 300){
                            is_to_sky_flag = reg_addr;
                            printf("reg write 0x%x: 0x%04x, set 300->1 for let cg cal to sky \n", reg_addr, reg_value);
                        }else{
                            dtof_write_reg_running(reg_addr, reg_value); // 示例：写入值为索引 i，你可根据实际需求改成 uart_buf 中解析的值
                            printf("reg write 0x%x: 0x%04x\n", reg_addr, reg_value);
                        }
                    }
                }
                else if (strcmp(uart_buf, "p") == 0)
                {
                    printf("chip uuid: ");
                    for (int i = 0; i < DTOF_UUID_LENGTH; i++)
                    {
                        printf("%d, ", dtof_get_chip_config()->chip_uuid[i]);
                    }
                    printf("\n");
                    dtof_ft_data_t ft_data_read;
                    dtof_bool_t is_legal_data = DTOF_FALSE;
                    dtof_get_ft_data_from_flash((dtof_uint16_t*)&ft_data_read, sizeof(dtof_ft_data_t)/sizeof(dtof_uint16_t), &is_legal_data);
                    if (is_legal_data == DTOF_FALSE)
                    {
                        printf("ft data is illegal, all 0xFF\n");
                    }
                    else
                    {
                        #ifdef DTOF_FT_CALIBRATE_BINOFFSET
                        printf("bin_offset = %u\n", ft_data_read.bin_offset);
                        #endif
                        #ifdef DTOF_FT_CALIBRATE_REFSPAD
                        printf("ref_spad = %u\n", ft_data_read.ref_spad);
                        #endif
                        #ifdef DTOF_FT_CALIBRATE_CG
                        printf("cg_reg: ");
                        dtof_uint16_t cg_reg;
                        for (int i = 0; i < (DTOF_AC_NUM + 1); i++)
                        {
                            cg_reg = ft_data_read.cg_data[i * 2 + 1] * 256 + ft_data_read.cg_data[i * 2];
                            printf("%u, ", cg_reg);
                        }
                        printf("\n");
                        #endif
                        #ifdef DTOF_FT_CALIBRATE_B
                        printf("distance_k=%d, distance_b=%d\n", ft_data_read.distance_k, ft_data_read.distance_b);
                        #endif
                        }
                }
                else if (strcmp(uart_buf, "clear") == 0)
                {
                    stm32_flash_write_init(DTOF_FT_DATA_FLASH_PAGE, DTOF_FT_DATA_FLASH_PAGE_NUM);
                }
                else if (strncmp(uart_buf, "ft,", 3) == 0)
                {
                    stm32_flash_write_init(DTOF_FT_DATA_FLASH_PAGE, DTOF_FT_DATA_FLASH_PAGE_NUM);
                    DTOF_CHECK_WARN(dtof_sensor_init(), "dtof sensor init failed\n");
                    dtof_uint16_t otp_ref_spad_mask,distance;

                    if (sscanf(uart_buf, "ft,%hu,%hu", &otp_ref_spad_mask, &distance) == 2)
                    {
                        DTOF_RET ret = dtof_do_ft_calibration(otp_ref_spad_mask, distance, is_to_sky_flag);
                        if (ret == DTOF_RET_SUCCESS) {
                            dtof_ft_data_t ft_data;
                            dtof_bool_t is_legal_data = DTOF_FALSE;
                            dtof_get_ft_data_from_flash((dtof_uint16_t*)&ft_data, sizeof(dtof_ft_data_t)/sizeof(dtof_uint16_t), &is_legal_data);
                            printf("FT success:\n");
                            if (is_legal_data != DTOF_TRUE)
                            {
                                printf("ft data is illegal\n");
                                continue;
                            }
                            #ifdef DTOF_FT_CALIBRATE_BINOFFSET
                            printf("bin_offset = %u\n", cal_data.binoffset_cal_data.binoffset);
                            #endif
                            #ifdef DTOF_FT_CALIBRATE_REFSPAD
                            printf("otp_ref_spad_mask = %u, ref_spad = %u\n", cal_data.ref_spad_cal.otp_ref_spad_mask, cal_data.ref_spad_cal.ref_spad);
                            #endif
                            #ifdef DTOF_FT_CALIBRATE_CG
                            printf("cg_reg: ");
                            dtof_uint16_t cg_reg;
                            for (int i = 0; i < (DTOF_AC_NUM + 1); i++)
                            {
                                cg_reg = ft_data.cg_data[i * 2 + 1] * 256 + ft_data.cg_data[i * 2];
                                printf("%u, ", cg_reg);
                            }
                            printf("\n");
                            #endif
                            #ifdef DTOF_FT_CALIBRATE_B
                            printf("distance = %u, distance_k=%.2f, distance_b=%.2f\n", cal_data.kb_data.far_distance, cal_data.kb_data.k, cal_data.kb_data.b);
                            #endif

                        }
                        else{
                            printf("ft calibration failed\n");
                        }
                    }
                }
                else if (strcmp(uart_buf, "v") == 0)
                {
                    DTOF_LOG("soc version: %s\n", SOC_VERSION_STRING);
                    DTOF_LOG("sdk version: %s\n", dtof_get_sdk_version());
                    DTOF_LOG("chip version: %d\n", DTOF_SWAP16(dtof_get_chip_version()));
                }
                else if (strcmp(uart_buf, "test") == 0)
                {
                    DTOF_LOG("enter test mode\n");
                    DTOF_CHECK_WARN(dtof_set_mcu_status_ram(DTOF_MCU_STATE_SLEEP_DIRECT), "set mcu sleep failed\n");
                    DTOF_CHECK_WARN(dtof_set_mcu_status_ram(DTOF_MCU_STATE_WAKEUP), "set mcu sleep failed\n");
                }
                else
                {
                    // printf("unknown command: %s\n", uart_buf);
                }

                // 清空缓冲区
                uart_index = 0;
                memset(uart_buf, 0, sizeof(uart_buf));
            }
            else
            {
                if (uart_index < sizeof(uart_buf) - 1)
                {
                    uart_buf[uart_index++] = byte;
                }
                else
                {
                    // 缓冲区溢出，重置
                    uart_index = 0;
                    memset(uart_buf, 0, sizeof(uart_buf));
                }
            }
        }

        if (is_init == DTOF_TRUE)
        {
#ifdef DTOF_INTERRUPT_MODE
            if (dtof_get_interrupt_flag() == DTOF_TRUE)
            {
                is_new_flag = DTOF_TRUE;
                dtof_get_distance_result(&distance_result);
                dtof_set_interrupt_flag(DTOF_FALSE);
            }
#elif defined(DTOF_POLLING_MODE)
            ret = dtof_get_distance_result_polling(&distance_result, &is_new_flag);
#endif
        }

        if (is_new_flag == DTOF_TRUE)
        {
            if (debug_flag == DTOF_TRUE)
            {
                if (frame_cnt_flag == DTOF_TRUE)
                {
                    frame_cnt++;
                    if (frame_cnt < 50)
                    {
                        goto PASS;
                    }
                }
                if (debug_flag == DTOF_TRUE)
                {
                    DTOF_CHECK_WARN(dtof_debug_mode_bypass(dev->chip_type), "debug mode bypass failed\n");
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

                if (dev->chip_type == DTOF_CHIP_TYPE_A05)
                {
                    dtof_io_interaction(0x30, 0x01);
                    DTOF_CHECK_WARN(dtof_io_interaction(DTOF_CMD_WRITE_REG_ADDR, SPECIAL_BYPASS_VALUE), "enable debug mode failed\n");
                }

                if (frame_cnt_flag == DTOF_TRUE)
                {
                    if (frame_cnt == 200)
                    {
                        debug_flag = DTOF_FALSE;
                        frame_cnt_flag = DTOF_FALSE;
                        frame_cnt = 0;
                        goto STOP_DISTANCE_MEASURE;
                    }
                }
            }
            printf("%d, %d, %d, %d, %.6f, 1\n",
                   distance_result.frame_id, distance_result.first_target, distance_result.first_intensity, distance_result.main_nflash, distance_result.ambient);
            is_new_flag = DTOF_FALSE;
        }
    PASS:
    {
    }
    }

    return 0;
}
