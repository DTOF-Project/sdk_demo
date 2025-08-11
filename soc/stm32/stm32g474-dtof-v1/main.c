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
#include "base/inc/mos_platform.h"
#include "data_base/sensor_database.h"
#include "customer/dtof_customer.h"

#include "dev/dtof_hal.h"

#include "user/device/ds_sal.h"
#include "user/device/ds_dev.h"
#include "user/device/device.h"

#include "application/inc/soc_version.h"


#define FLASH_START_ADDR 0x08070000
#define FLASH_RESERVE_SIZE 64
#define FLASH_ROM_BURN_START_ADDR 0x08070000 + FLASH_RESERVE_SIZE

/*****************************xyb改******************************/
extern DTOF_RET stm32_write_gpio(uint32_t gpio, uint32_t value);
extern DTOF_RET stm32_init_gpio(uint32_t gpio, uint32_t cfgset);
extern int stm32_uart_write(int uart_id, void *buf, int nbyte);
extern int stm32_uart_read(int uart_id, void *buf, int nbyte);

// 这里要求一定是输入的是 int16 的数据
void dump_hist_log(dtof_uint16_t* hist_p, dtof_uint16_t len){
    #define OUT_PUT_MAX_BUFFER  (517)
    char output_str[OUT_PUT_MAX_BUFFER]; // 确保缓冲区足够大
    char *temp_start = output_str;
    int total_used_len = 0;

    // 每个数据的单元大小是 5 = 4bytes hex + ，
    #define PRINT_UNIT_SIZE 5
    for (int i = 0; i < len; i++) {
        int pos;
        if(hist_p[i] <= 0xff)
        {
            pos = snprintf(temp_start , PRINT_UNIT_SIZE, "%x,", hist_p[i]);
        }
        else if (hist_p[i] <= 0xfff)
        {
            pos = snprintf(temp_start , PRINT_UNIT_SIZE+1, "%03x,", hist_p[i]);
        }
        else if (hist_p[i] <= 0xffff)
        {
            pos = snprintf(temp_start , PRINT_UNIT_SIZE+2, "%04x,", hist_p[i]);
        }
        total_used_len = total_used_len + pos;
        temp_start = temp_start + pos;

        if(total_used_len >= (OUT_PUT_MAX_BUFFER - PRINT_UNIT_SIZE))
        {
            // sendout the value
            stm32_uart_write(0, output_str, total_used_len);
            // reset the value
			temp_start = output_str;
		    total_used_len = 0;
        }
    }

    if(total_used_len != 0)
    {
        stm32_uart_write(0, output_str, total_used_len);
    }
    stm32_uart_write(0, "\n", 1);
}
/**************************************************************/

void stm32_flash_write_init(void)
{
    FLASH_EraseInitTypeDef eraseInit;
    uint32_t pageError = 0;

    // 解锁Flash
    if (HAL_FLASH_Unlock() != HAL_OK)
    {
        printf("flash unlock failed\n");
        return;
    }
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_OPTVERR);
    eraseInit.TypeErase = FLASH_TYPEERASE_PAGES;
    eraseInit.NbPages = 4; // 向上取整
    eraseInit.Page = 224;
    eraseInit.Banks = FLASH_BANK_2;

    do
    {
    } while (HAL_FLASHEx_Erase(&eraseInit, &pageError) != HAL_OK);
}

void stm32_flash_write_u64(uint32_t offset, uint64_t *context, uint16_t num_words)
{
    uint32_t flash_addr = FLASH_ROM_BURN_START_ADDR + offset;

    HAL_FLASH_Unlock();
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_OPTVERR);

    for (uint32_t i = 0; i < num_words; i++) {
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, flash_addr + i * 8, context[i]) != HAL_OK) {
            HAL_FLASH_Lock();
            return;
        }
    }

    HAL_FLASH_Lock();
}

void stm32_flash_read_u64(uint32_t offset, uint64_t *context, uint16_t num_words)
{
    uint32_t flash_addr = FLASH_ROM_BURN_START_ADDR + offset;
    for (uint32_t i = 0; i < num_words; i++) {
        context[i] = *(uint64_t *)(flash_addr + i * 8);
    }
}


void stm32_flash_write(uint32_t offset, uint64_t *context, uint16_t len)
{
    // 计算目标Flash地址
    uint32_t flash_addr = FLASH_ROM_BURN_START_ADDR + offset;
    uint32_t context_index = 0;

    do
    {

    } while (HAL_FLASH_Unlock() != HAL_OK);
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_OPTVERR);

    // 按8字节写入数据
    for (uint32_t i = 0; i < len; i += 4)
    {
        uint64_t data = *(context + context_index);
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, flash_addr + (context_index * 8), data) != HAL_OK)
        {
            HAL_FLASH_Lock();
            return;
        }
        context_index++;
    }

    HAL_FLASH_Lock();
}

void stm32_flash_read(uint32_t offset, uint64_t *context, uint16_t len)
{
    // 计算目标Flash地址
    // len 的单位是2byte
    // 起始地址跳过64的预留byte
    uint32_t flash_addr = FLASH_ROM_BURN_START_ADDR + offset;
    uint32_t context_index = 0;
    uint64_t data = 0;

    // 按8字节写入数据
    for (uint32_t i = 0; i < len; i += 4)
    {
        data = *(uint64_t *)(flash_addr + context_index * 8);
        *(context + context_index) = data;
        context_index++;
    }
}

// 从数组库中检索uuid
static DTOF_RET dtof_find_ft_data_in_database(dtof_uint8_t *uuid, dtof_uint8_t uuid_len, dtof_bool_t *is_find_sensor, dtof_uint8_t **sensor_ft_data_p)
{
    DTOF_RET ret = DTOF_RET_SUCCESS;
    dtof_int32_t sensor_index;

    *is_find_sensor = DTOF_FALSE;

    for (sensor_index = 0; sensor_index < sizeof(sensor_database) / sizeof(sensor_database_t); sensor_index++)
    {
        if (memcmp(uuid, sensor_database[sensor_index].uuid, uuid_len) == 0)
        {
            *is_find_sensor = DTOF_TRUE;
            *sensor_ft_data_p = (dtof_uint8_t *)&sensor_database[sensor_index];
            break;
        }
    }

    if (!(*is_find_sensor))
    {
        *sensor_ft_data_p = (dtof_uint8_t *)&sensor_database[0];
        DTOF_LOG_ERR("can not find uuid in database\n");
        ret = DTOF_RET_FAILED;
    }

    return ret;
}

#define SPECIAL_BYPASS_VALUE 7
#define SPECIAL_LOOP_VALUE   136
#define DTOF_ENABLE_DEBUG_MODE 1
#define DTOF_DISABLE_DEBUG_MODE 0
#define DTOF_WFI_STATUS_FLAG_ADDR 0x6e
#define DTOF_WFI_STATUS_FLAG 0xab
DTOF_RET dtof_set_debug_mode(dtof_int32_t debug_mode)
{
    if (debug_mode == DTOF_ENABLE_DEBUG_MODE) {
        DTOF_CHECK_RET(dtof_io_interaction(DTOF_CMD_WRITE_REG_ADDR, SPECIAL_BYPASS_VALUE), "enable debug mode failed\n");
    } else if (debug_mode == DTOF_DISABLE_DEBUG_MODE) {
        DTOF_CHECK_RET(dtof_io_interaction(DTOF_CMD_WRITE_REG_ADDR, SPECIAL_LOOP_VALUE), "disable debug mode failed\n");
    } else {
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
    // DTOF_RET ret = DTOF_RET_SUCCESS;
    // if (chip_type == DTOF_CHIP_TYPE_A05) {
    //     // a05使用inner mcu中断里的bypass, 偶发会导致inner mcu crash, 需要特殊处理, 使用外部的bypass, 且bypass前关闭timer, dsp, eyesafe中断, 进入wfi, 唤醒后
    //     dtof_uint16_t intr_control_flag;
    //     dtof_uint16_t bypassvalue = 0x17b9;
    //     DTOF_CHECK_RET(dtof_read_innermcu_intr_control_flag(&intr_control_flag), "read inner mcu status failed\n");
    //     if (intr_control_flag != DTOF_WFI_STATUS_FLAG)
    //     {
    //         DTOF_LOG_ERR("intr_control_flag is 0x%x\n", intr_control_flag);
    //         ret = DTOF_RET_FAILED;
    //     }

    //     DTOF_CHECK_RET(dtof_reg_burst_write(DTOF_IO_CTRL_REG_ADDR, &bypassvalue, 1), "write bypass value failed\n");
    // } else if (chip_type == DTOF_CHIP_TYPE_L3) {
    //     DTOF_CHECK_RET(dtof_set_mcu_status(DTOF_MCU_STATE_SLEEP_DIRECT), "set mcu sleep failed\n");
    // } else {
    //     DTOF_LOG_ERR("unknown chip type\n");
    //     ret = DTOF_RET_FAILED;
    // }
    // return ret;
    DTOF_CHECK_RET(dtof_set_mcu_status(DTOF_MCU_STATE_SLEEP_DIRECT), "set mcu sleep failed\n");
    return DTOF_RET_SUCCESS;
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
                    if (dev->chip_type == DTOF_CHIP_TYPE_A05)
                    {
                        DTOF_CHECK_WARN(dtof_io_interaction(DTOF_CMD_WRITE_REG_ADDR, SPECIAL_BYPASS_VALUE), "enable debug mode failed\n");
                    }
                    dtof_start_distance_measure();
                    is_init = DTOF_TRUE;
                    debug_flag = DTOF_TRUE;
                }
                else if (strcmp(uart_buf, "e") == 0)
                {
                    DTOF_CHECK_WARN(dtof_sensor_init(), "dtof sensor init failed\n");
                    if (dev->chip_type == DTOF_CHIP_TYPE_A05)
                    {
                        DTOF_CHECK_WARN(dtof_io_interaction(DTOF_CMD_WRITE_REG_ADDR, SPECIAL_BYPASS_VALUE), "enable debug mode failed\n");
                    }
                    dtof_start_distance_measure();
                    frame_cnt_flag = DTOF_TRUE;
                    is_init = DTOF_TRUE;
                    debug_flag = DTOF_TRUE;
                }
                else if (strcmp(uart_buf, "t") == 0)
                {
                    dtof_stop_distance_measure();
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
                    DTOF_CHECK_RET(dtof_set_mcu_status(DTOF_MCU_STATE_SLEEP_DIRECT), "set mcu sleep failed\n");
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
                    DTOF_CHECK_RET(dtof_set_mcu_status(DTOF_MCU_STATE_WAKEUP), "wakeup mcu failed\n");
                }
                else if (strncmp(uart_buf, "w,", 2) == 0)
                {
                    int reg_addr, reg_value;
                    if (sscanf(uart_buf, "w,%d,%d", &reg_addr, &reg_value) == 2)
                    {
                        dtof_write_reg_running(reg_addr, reg_value); // 示例：写入值为索引 i，你可根据实际需求改成 uart_buf 中解析的值
                        printf("reg write 0x%x: 0x%04x\n", reg_addr, reg_value);
                    }
                }
                else if (strcmp(uart_buf, "p") == 0)
                {
                    dtof_uint8_t chip_uuid[DTOF_UUID_LENGTH];
                    // dtof_int32_t read_distance_offset = 0;
                    // dtof_uint16_t xtalk_data_read[XTALK_DATA_SIZE];
                    DTOF_CHECK_WARN(dtof_get_uuid(chip_uuid, DTOF_UUID_LENGTH), "get uuid failed\n");
                    printf("chip uuid: ");
                    for (int i = 0; i < DTOF_UUID_LENGTH; i++)
                    {
                        printf("%d, ", chip_uuid[i]);
                    }
                    printf("\n");
                    // dtof_get_distance_offset_from_flash(&read_distance_offset);
                    // printf("distance offset = %d\n", read_distance_offset);
                    // dtof_get_xtalk_data_from_flash(xtalk_data_read);
                    // printf("xtalk data = ");
                    // for (int i = 0; i < XTALK_DATA_SIZE; i++)
                    // {
                    //     printf("%d, ", xtalk_data_read[i]);
                    // }
                    // printf("\n");
                }
                else if (strcmp(uart_buf, "cal") == 0)
                {
                    // DTOF_CHECK_WARN(dtof_init_and_wait_for_ready(&chip_id, DO_OFFSET_CALIBRATION_MODE), "dtof init and wait for ready failed\n");
                    // is_init = DTOF_TRUE;
                    // printf("distance offset = %d\n", dtof_get_distance_offset());
                }
                else if (strcmp(uart_buf, "clear") == 0)
                {
                    stm32_flash_write_init();
                }
                else if (strcmp(uart_buf, "b") == 0)
                {
                //     stm32_flash_write_init();
                //     DTOF_CHECK_WARN(dtof_init_and_wait_for_ready(&chip_id, DO_XTALK_CALIBRATION_MODE), "dtof init and wait for ready failed\n");
                //     uint16_t xtalk_data[18];
                //     dtof_get_xtalk_data_from_flash(xtalk_data);
                    // is_init = DTOF_TRUE; // cg 和 b 都校准完才视为校准完成
                }
                else if (strcmp(uart_buf, "v") == 0)
                {
                    DTOF_LOG("soc version: %s\n", SOC_VERSION_STRING);
                    DTOF_LOG("sdk version: %s\n", dtof_get_sdk_version());
                    DTOF_LOG("chip version: %d\n", DTOF_SWAP16(dtof_get_chip_version()));
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
                DTOF_CHECK_WARN(dtof_debug_mode_bypass(dev->chip_type), "debug mode bypass failed\n");
                if (frame_cnt_flag == DTOF_TRUE)
                {
                    frame_cnt++;
                    if (frame_cnt < 50)
                    {
                        goto PASS;
                    }
                    if (frame_cnt == 200)
                    {
                        debug_flag = DTOF_FALSE;
                        frame_cnt = 0;
                        dtof_stop_distance_measure();
                    }
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
