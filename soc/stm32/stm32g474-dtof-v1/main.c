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
#include "inc/dtof_common.h"
#include "inc/dtof_calibration_ft.h"
#include "inc/dtof_global_config.h"
#include "inc/dev/dtof_dev_api.h"
#include "base/inc/mos_platform.h"
#include "data_base/sensor_database.h"
#include "customer/dtof_customer.h"

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
    dtof_uint16_t reg80;
    dtof_bool_t is_new_flag;
    dtof_bool_t first_new_flag = DTOF_TRUE; // polling模式下, 不取第一帧, debug模式下需要特殊处理, 因为新的bypass交互流程
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
    char uart_buf[128] = {0};
    uint8_t uart_index = 0;

    dtof_bool_t frame_cnt_flag = DTOF_FALSE;
    int32_t frame_cnt = 0;

    dtof_set_interrupt_flag(DTOF_FALSE);

    while (1)
    {
        is_new_flag = DTOF_FALSE;
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
                    dtof_reg_burst_read(80, &reg80, 1);
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
                    dtof_reg_burst_read(80, &reg80, 1);
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
                else if (strncmp(uart_buf, "ri,", 3) == 0)
                {
                    int reg_addr = atoi(&uart_buf[3]);
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
                else if (strncmp(uart_buf, "wi,", 3) == 0)
                {
                    int reg_addr, reg_value;
                    if (sscanf(uart_buf, "wi,%d,%d", &reg_addr, &reg_value) == 2)
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
                    printf("otp list:\n");
                    dtof_uint8_t otp_data[128];
                    dtof_set_mcu_status_ram(DTOF_MCU_STATE_SLEEP_DIRECT);
                    DTOF_CHECK_RET(dtof_read_otp(0, otp_data, 128), "read otp failed\n");
                    dtof_set_mcu_status_ram(DTOF_MCU_STATE_WAKEUP);
                    for (int i = 0; i < 128; i++)
                    {
                        printf("%d, ", otp_data[i]);
                    }
                    printf("\n");
                    // dtof_read_otp
                    dtof_ft_cali_param_t ft_data_read;
                    dtof_bool_t is_legal_data = DTOF_FALSE;
                    dtof_get_ft_data_from_flash((dtof_uint16_t*)&ft_data_read, sizeof(dtof_ft_cali_param_t)/sizeof(dtof_uint16_t), &is_legal_data);
                    if (is_legal_data == DTOF_FALSE)
                    {
                        dtof_printf("ft data is illegal, all 0xFF\n");
                    }
                    else
                    {
                        dtof_uint16_t ft_cali_type = ft_data_read.ft_calibration_type;
                        dtof_printf("ft cali type: 0x%04x\n", ft_cali_type);
                        if(DTOF_BIT_GET(ft_cali_type, DTOF_FT_CALIBRATE_BINOFFSET))
                        {
                            dtof_printf("bin_offset = %u\n", ft_data_read.dtof_ft_data.bin_offset);
                        }
                        if(DTOF_BIT_GET(ft_cali_type, DTOF_FT_CALIBRATE_REFSPAD))
                        {
                            dtof_printf("ref_spad = %u\n", ft_data_read.dtof_ft_data.ref_spad);
                        }
                        if(DTOF_BIT_GET(ft_cali_type, DTOF_FT_CALIBRATE_CG))
                        {
                            dtof_printf("cg_reg: ");
                            dtof_uint16_t cg_reg;
                            for (int i = 0; i < (DTOF_AC_NUM + 1); i++)
                            {
                                cg_reg = ft_data_read.dtof_ft_data.cg_data[i * 2 + 1] * 256 + ft_data_read.dtof_ft_data.cg_data[i * 2];
                                dtof_printf("%u, ", cg_reg);
                            }
                            dtof_printf("\n");
                        }
                        if(DTOF_BIT_GET(ft_cali_type, DTOF_FT_CALIBRATE_B))
                        {
                            dtof_printf("distance_k=%d, distance_b=%d\n", ft_data_read.dtof_ft_data.distance_k, ft_data_read.dtof_ft_data.distance_b);
                        }
                    }
                    dtof_parse_inner_mcu_error_code();
                }
                else if (strcmp(uart_buf, "clear") == 0)
                {
                    stm32_flash_write_init(DTOF_FT_DATA_FLASH_PAGE, DTOF_FT_DATA_FLASH_PAGE_NUM);
                }
                else if (strncmp(uart_buf, "ft,", 3) == 0)
                {
                    dtof_uint16_t ft_cali_type;
                    dtof_uint16_t ft_actual_param;

                    if (sscanf(uart_buf, "ft,%hu,%hu", &ft_cali_type, &ft_actual_param) == 2)
                    {
                        dtof_printf("start ft calibration, type=0x%04x\n", ft_cali_type);
                        DTOF_RET ret = dtof_do_ft_calibration(ft_cali_type, ft_actual_param);
                        if (ret == DTOF_RET_SUCCESS) {
                            dtof_ft_cali_param_t ft_cali_param;
                            dtof_bool_t is_legal_data = DTOF_FALSE;
                            dtof_get_ft_data_from_flash((dtof_uint16_t*)&ft_cali_param, sizeof(dtof_ft_cali_param_t)/sizeof(dtof_uint16_t), &is_legal_data);
                            dtof_printf("FT success:\n");
                            if (is_legal_data != DTOF_TRUE)
                            {
                                dtof_printf("ft data is illegal\n");
                                continue;
                            }
                            if(DTOF_BIT_GET(ft_cali_type, DTOF_FT_CALIBRATE_BINOFFSET))
                            {
                                dtof_printf("bin_offset = %u\n", ft_cali_param.dtof_ft_data.bin_offset);
                            }
                            if(DTOF_BIT_GET(ft_cali_type, DTOF_FT_CALIBRATE_REFSPAD))
                            {
                                dtof_printf("ref_spad = %u\n", ft_cali_param.dtof_ft_data.ref_spad);
                            }
                            if(DTOF_BIT_GET(ft_cali_type, DTOF_FT_CALIBRATE_CG))
                            {
                                dtof_printf("cg_reg: ");
                                dtof_uint16_t cg_reg;
                                for (int i = 0; i < (DTOF_AC_NUM + 1); i++)
                                {
                                    cg_reg = ft_cali_param.dtof_ft_data.cg_data[i * 2 + 1] * 256 + ft_cali_param.dtof_ft_data.cg_data[i * 2];
                                    dtof_printf("%u, ", cg_reg);
                                }
                                dtof_printf("\n");
                            }
                            if(DTOF_BIT_GET(ft_cali_type, DTOF_FT_CALIBRATE_B))
                            {
                                dtof_printf("distance_k=%d, distance_b=%d\n", ft_cali_param.dtof_ft_data.distance_k, ft_cali_param.dtof_ft_data.distance_b);
                            }
                        }
                        else{
                            dtof_printf("ft calibration failed\n");
                        }
                    }
                }
                else if (strcmp(uart_buf, "v") == 0)
                {
                    DTOF_LOG("soc version: %s\n", SOC_VERSION_STRING);
                    DTOF_LOG("sdk version: %s\n", dtof_get_sdk_version());
                    DTOF_LOG("chip version: %d\n", DTOF_SWAP16(dtof_get_chip_version()));
                }
                else if (strcmp(uart_buf, "init") == 0)
                {
                    dtof_init_device_info();
                    DTOF_CHECK_WARN(dtof_sensor_init(), "dtof sensor init failed\n");
                }
                else if (strcmp(uart_buf, "uuid") == 0)
                {
                    dtof_uint8_t chip_uuid_buffer[DTOF_UUID_LENGTH];
                    DTOF_CHECK_WARN(dtof_get_uuid(chip_uuid_buffer, DTOF_UUID_LENGTH), "get uuid failed\n");
                    printf("chip uuid: ");
                    for (int i = 0; i < DTOF_UUID_LENGTH; i++)
                    {
                        printf("%d, ", chip_uuid_buffer[i]);
                    }
                }
                else if (strcmp(uart_buf, "error_code") == 0)
                {
                    dtof_uint32_t error_code;
                    DTOF_CHECK_WARN(dtof_get_error_info(&error_code), "get error info failed\n");
                    printf("error code: 0x%x\n", error_code);
                }
                else if (strncmp(uart_buf, "osc_cal,", 8) == 0)
                {
                    int osc_cal_mode;
                    if (sscanf(uart_buf, "osc_cal,%d", &osc_cal_mode) == 1)
                    {
                        DTOF_CHECK_WARN(dtof_write_reg_running(DTOF_FRAME_CONTROL_REG, (osc_cal_mode << 12) | 0x0388), "dtof start failed\n");
                    }
                }
                else if (strncmp(uart_buf, "wft,", 4) == 0)
                {
                #define FT_MAX_NUM 39 // type + 34cg+ binoffset + k + b + refspad
                    char buf_copy[128];
                    strncpy(buf_copy, uart_buf, sizeof(buf_copy));
                    buf_copy[sizeof(buf_copy) - 1] = '\0';

                    char *token = strtok(buf_copy, ","); // 第一个 token: "r18"
                    int values[FT_MAX_NUM];
                    int count = 0;

                    while ((token = strtok(NULL, ",")) != NULL) {
                        if (count >= FT_MAX_NUM) {
                            printf("Too many numbers, need exactly 39\n");
                            continue;
                        }

                        char *endptr;
                        long val = strtol(token, &endptr, 10);
                        if (*endptr != '\0') {
                            printf("Invalid number: %s\n", token);
                            continue;
                        }

                        values[count++] = (int)val;
                    }

                    if (count < 2) {
                        printf("count < 2, = %d\n", count);
                        goto CLEAR_REV_BUFFER;
                    }

                    dtof_uint16_t ft_cali_type;
                    dtof_ft_cali_param_t ft_cali_param;
                    dtof_bool_t is_legal_ft_data = DTOF_FALSE;

                    DTOF_CHECK_WARN(dtof_get_ft_data_from_flash((dtof_uint16_t *)&ft_cali_param, sizeof(dtof_ft_cali_param_t) / sizeof(dtof_uint16_t), &is_legal_ft_data), "get ft data from flash failed\n");

                    ft_cali_type = values[0];

                    if(DTOF_BIT_GET(ft_cali_type, DTOF_FT_CALIBRATE_BINOFFSET))
                    {
                        ft_cali_param.dtof_ft_data.bin_offset = values[1];
                        printf("set binoffset = %d\n", values[1]);
                    }
                    if(DTOF_BIT_GET(ft_cali_type, DTOF_FT_CALIBRATE_REFSPAD))
                    {
                        ft_cali_param.dtof_ft_data.ref_spad = values[1];
                        printf("set refspad = %d\n", values[1]);
                    }
                    if(DTOF_BIT_GET(ft_cali_type, DTOF_FT_CALIBRATE_CG))
                    {
                        if (count < 18) {
                            printf("count < 18, = %d\n", count);
                            goto CLEAR_REV_BUFFER;
                        }

                        printf("set cg: ");
                        for (int i = 0; i < (DTOF_AC_NUM + 1); i++)
                        {
                            ft_cali_param.dtof_ft_data.cg_data[i * 2] = values[i + 1] & 0xff;
                            ft_cali_param.dtof_ft_data.cg_data[i * 2 + 1] = (values[i + 1] & 0xff00) >> 8;
                            printf("%d, ", values[i + 1]);
                        }
                        printf("\n");
                    }
                    if(DTOF_BIT_GET(ft_cali_type, DTOF_FT_CALIBRATE_B))
                    {
                        ft_cali_param.dtof_ft_data.distance_k = 1197;
                        ft_cali_param.dtof_ft_data.distance_b = values[1];
                        printf("set b value = %d\n", values[1]);
                    }

                    if (is_legal_ft_data == DTOF_TRUE) {
                        ft_cali_param.ft_calibration_type |= ft_cali_type;
                    } else {
                        ft_cali_param.ft_calibration_type = ft_cali_type;
                    }
                    DTOF_CHECK_RET(dtof_set_ft_data((dtof_uint16_t*)&ft_cali_param), "set ft data failed\n");
                    dtof_set_ft_calibration_type(ft_cali_param.ft_calibration_type);
                    DTOF_CHECK_RET(dtof_set_ft_data_to_flash((dtof_uint16_t*)&ft_cali_param, sizeof(ft_cali_param) / sizeof(dtof_uint16_t)), "set ft data to flash failed\n");
                }
                else
                {
                    // printf("unknown command: %s\n", uart_buf);
                }
CLEAR_REV_BUFFER:
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

#ifdef DTOF_POLLING_MODE
        if (dev->chip_type == DTOF_CHIP_TYPE_A05)
        {
            if (first_new_flag == DTOF_TRUE)
            {
                if (debug_flag == DTOF_TRUE)
                {
                    if ((reg80 != distance_result.frame_id) && (is_new_flag != DTOF_TRUE))
                    {
                        dtof_io_interaction(0x30, 0x01);
                        DTOF_CHECK_WARN(dtof_io_interaction(DTOF_CMD_WRITE_REG_ADDR, SPECIAL_BYPASS_VALUE), "enable debug mode failed\n");
                        first_new_flag = DTOF_FALSE;
                    }
                }
            }
        }
#endif

        if (is_new_flag == DTOF_TRUE)
        {
            if (debug_flag == DTOF_TRUE)
            {
                if (frame_cnt_flag == DTOF_TRUE)
                {
                    frame_cnt++;
                    if (frame_cnt < 0)
                    {
                        if (dev->chip_type == DTOF_CHIP_TYPE_A05)
                        {
                            dtof_io_interaction(0x30, 0x01);
                            DTOF_CHECK_WARN(dtof_io_interaction(DTOF_CMD_WRITE_REG_ADDR, SPECIAL_BYPASS_VALUE), "enable debug mode failed\n");
                        }
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
                    if (frame_cnt == 150)
                    {
                        debug_flag = DTOF_FALSE;
                        frame_cnt_flag = DTOF_FALSE;
                        frame_cnt = 0;
                        printf("%d, %d, %d, %d, %.6f, 1\n",
                            distance_result.frame_id, distance_result.first_target, distance_result.first_intensity, distance_result.main_nflash, distance_result.ambient);
                        is_new_flag = DTOF_FALSE;
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
