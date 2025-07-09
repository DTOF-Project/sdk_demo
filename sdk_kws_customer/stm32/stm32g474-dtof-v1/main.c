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
#include "lib/dtof_lib.h"


#define FLASH_START_ADDR 0x08070000
#define FLASH_RESERVE_SIZE 64
#define FLASH_ROM_BURN_START_ADDR 0x08070000 + FLASH_RESERVE_SIZE

/*****************************xyb改******************************/
extern DTOF_RET stm32_write_gpio(uint32_t gpio, uint32_t value);
extern DTOF_RET stm32_init_gpio(uint32_t gpio, uint32_t cfgset);
extern int stm32_uart_write(int uart_id, void *buf, int nbyte);

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

/**
 * @brief
 * @return int
 */
int main(void)
{
    DTOF_RET ret;
    dtof_uint16_t chip_id;
    dtof_uint8_t device_id = 0;

    // dtof_uint8_t uuid[DTOF_UUID_LENGTH];
    // dtof_uint8_t *sensor_ft_data_p = 0;
    // dtof_bool_t is_find_sensor = DTOF_FALSE;

    dtof_uint16_t buffer[DTOF_SINGLE_MAIN_HISTGRAM_LEN + 64];
    dtof_distance_result_t distance_result;
    dtof_bool_t is_new_flag;
    dtof_bool_t is_init = DTOF_FALSE;
    dtof_bool_t debug_flag = DTOF_FALSE;

    DTOF_CHECK_WARN(platform_init(), "platform init failed\n");

    DTOF_CHECK_WARN(dtof_peripheral_device_init(), "dtof_peripheral_device_init failed\n");

    // dtof_int32_t write_value = 20;
    // dtof_int32_t read_value;
    // stm32_flash_write(0, (uint64_t*)&write_value, 1);
    // stm32_flash_read(0, (uint64_t*)&read_value, 1);
    // printf("write value: %d, read value: %d\n", write_value, read_value);

    // for test
    // DTOF_CHECK_WARN(dtof_get_uuid(device_id, uuid, DTOF_UUID_LENGTH), "get uuid failed\n");

    // ret = dtof_find_ft_data_in_database(uuid, DTOF_UUID_LENGTH, &is_find_sensor, &sensor_ft_data_p);

    // if (!ret)
    // {
    //     dtof_quit_distance_measure(device_id);
    //     printf("load ft data... ...\n");
    //     DTOF_CHECK_WARN(dtof_version_upgrade(device_id, uuid, sensor_ft_data_p, DTOF_SENSOR_DATA_LENGTH), "version upgrade failed\n");
    // }

    // dtof_set_distance_offset(device_id, -4);

    extern int stm32_uart_read(int uart_id, void *buf, int nbyte);

    uint8_t byte;
    char uart_buf[32] = {0};
    uint8_t uart_index = 0;

    dtof_bool_t frame_cnt_flag = DTOF_FALSE;
    int32_t frame_cnt = 0;

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
                    DTOF_CHECK_WARN(dtof_init_and_wait_for_ready(device_id, &chip_id, NORMAL_DISTANCE_MODE), "dtof init and wait for ready failed\n");
                    is_init = DTOF_TRUE;
                    dtof_start_distance_measure(device_id);
                    debug_flag = DTOF_FALSE;
                }
                else if (strcmp(uart_buf, "d") == 0)
                {
                    DTOF_CHECK_WARN(dtof_init_and_wait_for_ready(device_id, &chip_id, NORMAL_DISTANCE_MODE), "dtof init and wait for ready failed\n");
                    is_init = DTOF_TRUE;
                    dtof_start_distance_measure(device_id);
                    debug_flag = DTOF_TRUE;
                }
                else if (strcmp(uart_buf, "e") == 0)
                {
                    DTOF_CHECK_WARN(dtof_init_and_wait_for_ready(device_id, &chip_id, NORMAL_DISTANCE_MODE), "dtof init and wait for ready failed\n");
                    is_init = DTOF_TRUE;
                    dtof_start_distance_measure(device_id);
                    frame_cnt_flag = DTOF_TRUE;
                    debug_flag = DTOF_TRUE;
                }
                else if (strcmp(uart_buf, "t") == 0)
                {
                    dtof_stop_distance_measure(device_id);
                }
                else if (strncmp(uart_buf, "c,", 2) == 0)
                {
                    int value = atoi(&uart_buf[2]);
                    printf("receive calbration value: %d\n", value);
                    dtof_set_distance_offset(device_id, 20 - value);
                    // 这里可以执行设置参数的操作
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
                    DTOF_CHECK_RET(dtof_set_mcu_status(device_id, DTOF_MCU_STATE_SLEEP_DIRECT), "set mcu sleep failed\n");
                    int reg_addr, reg_num;
                    dtof_uint16_t reg_max[255];
                    if (sscanf(uart_buf, "rb,%d,%d", &reg_addr, &reg_num) == 2)
                    {
                        dtof_reg_burst_read(device_id, reg_addr, reg_max, reg_num);
                        printf("burst reg read 0x%04x:\n", reg_addr);
                        for (int i = 0; i < reg_num; i++)
                        {
                            printf("%d, ", reg_max[i]);
                        }
                        printf("\n");
                    }
                    DTOF_CHECK_RET(dtof_set_mcu_status(device_id, DTOF_MCU_STATE_WAKEUP), "wakeup mcu failed\n");
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
                    dtof_int32_t read_distance_offset = 0;
                    dtof_uint16_t xtalk_data_read[XTALK_DATA_SIZE];
                    DTOF_CHECK_WARN(dtof_get_uuid(device_id, chip_uuid, DTOF_UUID_LENGTH), "get uuid failed\n");
                    printf("chip uuid: ");
                    for (int i = 0; i < DTOF_UUID_LENGTH; i++)
                    {
                        printf("%d, ", chip_uuid[i]);
                    }
                    printf("\n");
                    dtof_get_distance_offset_from_flash(device_id, &read_distance_offset);
                    printf("distance offset = %d\n", read_distance_offset);
                    dtof_get_xtalk_data_from_flash(device_id, xtalk_data_read);
                    printf("xtalk data = ");
                    for (int i = 0; i < XTALK_DATA_SIZE; i++)
                    {
                        printf("%d, ", xtalk_data_read[i]);
                    }
                    printf("\n");
                }
                else if (strcmp(uart_buf, "cal") == 0)
                {
                    DTOF_CHECK_WARN(dtof_init_and_wait_for_ready(device_id, &chip_id, DO_OFFSET_CALIBRATION_MODE), "dtof init and wait for ready failed\n");
                    is_init = DTOF_TRUE;
                    printf("distance offset = %d\n", dtof_get_distance_offset(device_id));
                }
                else if (strcmp(uart_buf, "clear") == 0)
                {
                    stm32_flash_write_init();
                }
                else if (strcmp(uart_buf, "b") == 0)
                {
                    stm32_flash_write_init();
                    DTOF_CHECK_WARN(dtof_init_and_wait_for_ready(device_id, &chip_id, DO_XTALK_CALIBRATION_MODE), "dtof init and wait for ready failed\n");
                    uint16_t xtalk_data[18];
                    dtof_get_xtalk_data_from_flash(device_id, xtalk_data);
                    // is_init = DTOF_TRUE; // cg 和 b 都校准完才视为校准完成
                }
                else if (strcmp(uart_buf, "x") == 0)
                {
                    DTOF_CHECK_RET(dtof_set_mcu_status(device_id, DTOF_MCU_STATE_SLEEP_DIRECT), "set mcu sleep failed\n");
                    #define READ_LEN 1126
                    dtof_uint16_t ram_start = 0x2000;
                    dtof_uint16_t ram_read[READ_LEN];
                    DTOF_CHECK_RET(dtof_reg_burst_write(device_id, 0XFE, &ram_start, 1),
                                    "写入串扰数据失败");
                    DTOF_CHECK_RET(dtof_reg_burst_read(device_id, 0xff, ram_read, READ_LEN),
                                    "读取距离结果失败");

                    printf("ramdata\n");
                    for(int i = 0; i < READ_LEN; i++)
                    {
                        printf("0x%04x, ", ram_read[i]);
                        if ((i + 1) % 16 == 0){
                            printf("\n");
                        }
                    }
                    printf("\n");

                    DTOF_CHECK_RET(dtof_set_mcu_status(device_id, DTOF_MCU_STATE_WAKEUP), "wakeup mcu failed\n");
                }
                else if (strcmp(uart_buf, "v") == 0)
                {
                    DTOF_LOG("sdk version: %s\n", dtof_get_sdk_version());
                    DTOF_LOG("chip version: %d\n", DTOF_SWAP16(dtof_get_chip_version(device_id)));
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
            // 检查是否有中断触发
            ret = dtof_get_distance_result(device_id, NORMAL_DISTANCE_MODE, &distance_result, &is_new_flag);
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
                    if (frame_cnt == 200)
                    {
                        debug_flag = DTOF_FALSE;
                        frame_cnt = 0;
                        dtof_stop_distance_measure(device_id);
                    }
                }
                // dtof_uint16_t main_hist[DTOF_SINGLE_MAIN_HISTGRAM_LEN];
                // dtof_uint16_t ref_hist[DTOF_SINGLE_REF_HISTGRAM_LEN];
                // dtof_uint16_t dsp_fifo[DTOF_SINGLE_FIFO_LEN];
                // #define TOTAL_REG_NUM 255
                // dtof_uint16_t dtof_reg[TOTAL_REG_NUM];
                // bypass, read debug info
                #define TOTAL_REG_NUM 255
                dtof_set_mcu_status(device_id, DTOF_MCU_STATE_SLEEP_DIRECT);

                dtof_histgram_io_read(DTOF_SINGLE_MAIN_HISTGRAM_OFFSET, buffer, DTOF_SINGLE_MAIN_HISTGRAM_LEN);
                dump_hist_log(buffer, DTOF_SINGLE_MAIN_HISTGRAM_LEN);
                dtof_histgram_io_read(DTOF_SINGLE_REF_HISTGRAM_OFFSET, buffer, DTOF_SINGLE_REF_HISTGRAM_LEN);
                dump_hist_log(buffer, DTOF_SINGLE_REF_HISTGRAM_LEN);
                dtof_dsp_fifo_read(0, buffer, DTOF_SINGLE_FIFO_LEN);
                dump_hist_log(buffer, DTOF_SINGLE_FIFO_LEN);
                dtof_reg_burst_read(0, 0x00, buffer, TOTAL_REG_NUM);
                dump_hist_log(buffer, TOTAL_REG_NUM);

                // printf("main histgram: ");
                // for (int i = 0; i < DTOF_SINGLE_MAIN_HISTGRAM_LEN; i++)
                // {
                //     printf("%d, ", main_hist[i]);
                // }
                // printf("\n");
                // // printf("ref histgram: ");
                // for (int i = 0; i < DTOF_SINGLE_REF_HISTGRAM_LEN; i++)
                // {
                //     printf("%d, ", ref_hist[i]);
                // }
                // printf("\n");
                // // printf("dsp fifo: ");
                // for (int i = 0; i < DTOF_SINGLE_FIFO_LEN; i++)
                // {
                //     printf("%d, ", dsp_fifo[i]);
                // }
                // printf("\n");
                // // printf("cg reg: ");
                // for (int i = 0; i < TOTAL_REG_NUM; i++)
                // {
                //     printf("%d, ", dtof_reg[i]);
                // }
                // printf("\n");

                dtof_set_mcu_status(device_id, DTOF_MCU_STATE_WAKEUP);
            }
            printf("%d, %d, %d, %d, %.6f, %d\n",
                   distance_result.frame_id, distance_result.first_target, distance_result.first_intensity, distance_result.main_nflash, distance_result.ambient, distance_result.is_legal_frame);
        }
    PASS:
    {

    }
    }

    return 0;
}
