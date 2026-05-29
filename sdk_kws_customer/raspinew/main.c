#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <time.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <signal.h>
#include <string.h>
#define DTOF_POLLING_MODE
#include "i2c_init.h"
// #include <cJSON.h>  // 添加在文件开头的其他include语句之后
#include <openssl/buffer.h>  // Add this for BUF_MEM
#include <openssl/bio.h>
#include <openssl/evp.h>
#include "gpio_init.h"
#include "serial_init.h"
#include "dtof_hal.h"
#include "sdk/inc/dtof_log.h"
#include "sdk/inc/dtof_api.h"
#include "sdk/inc/dtof_driver.h"
#include "sdk/inc/dtof_endian.h"
#include "sdk/inc/dtof_global_config.h"
#include "raspinew/dtof_customer.h"
#include "sdk/inc/dtof_calibration_ft.h"
#include "sdk/inc/lib/dtof_ft.h"
// #include "stm32/customer/dtof_customer.h"
// #include "stm32/dev/dtof_hal.c"
// #define DTOF_CHIP_TYPE_L3 0x4120
// #define DTOF_CHIP_TYPE_A05 0x0001
#define XTALK_DATA_SIZE 18
#define DO_OFFSET_CALIBRATION_MODE 2
#define DO_XTALK_CALIBRATION_MODE 1
#define CMD_BUFFER_SIZE 256

dtof_device_info_t dtof_device_info={
    .chip_is_init =DTOF_FALSE,
    .first_frame=1,
    .frame_id_pre=0,
    .ft_calibration_type =0
};
// void dtof_set_ft_calibration_type(dtof_uint16_t type)
// {
//     dtof_device_info.ft_calibration_type = type;
// }
#define SPECIAL_BYPASS_VALUE 7
#define SPECIAL_LOOP_VALUE 136
#define DTOF_ENABLE_DEBUG_MODE 1
#define DTOF_DISABLE_DEBUG_MODE 0
#define DTOF_WFI_STATUS_FLAG_ADDR 0x6e
#define DTOF_WFI_STATUS_FLAG 0xab
DTOF_RET dtof_read_innermcu_intr_control_flag(dtof_uint16_t *intr_control_flag)
{
    DTOF_CHECK_RET(dtof_reg_burst_read(DTOF_WFI_STATUS_FLAG_ADDR, intr_control_flag, 1), "read reg 0x6e failed\n");

    return DTOF_RET_SUCCESS;
}
DTOF_RET dtof_debug_mode_bypass(dtof_uint16_t chip_type)
{
    DTOF_RET ret = DTOF_RET_SUCCESS;
    if (chip_type == DTOF_A05_CHIPID)
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
    else if (chip_type == DTOF_L3_CHIPID)
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
void parse_inner_mcu_error_code(dtof_uint32_t error_code)
{
#define SYSTEM_ERROR_OTP_CPT_CHECKFAILED  0
#define SYSTEM_ERROR_OTP_FT_CHECKFAILED  1
#define SYSTEM_ERROR_OTP_USERMODE_CHECKFAILED  2
#define SYSTEM_WDT_RESET  3
#define SYSTEM_05_REG_RESET  4
#define SYSTEM_ERROR_EYESAFETY  5
#define SYSTEM_ERROR_OTP_CPA_CHECKFAILED  6
#define SYSTEM_SMOKE_STAGE1_ERROR  7
    typedef struct {
        dtof_uint8_t bit;
        const char *msg;
    } error_info_t;

    static const error_info_t error_table[] = {
        {SYSTEM_ERROR_OTP_CPT_CHECKFAILED, "SYSTEM_ERROR_OTP_CPT_CHECKFAILED"},
        {SYSTEM_ERROR_OTP_FT_CHECKFAILED, "SYSTEM_ERROR_OTP_FT_CHECKFAILED"},
        {SYSTEM_ERROR_OTP_USERMODE_CHECKFAILED, "SYSTEM_ERROR_OTP_USERMODE_CHECKFAILED"},
        {SYSTEM_WDT_RESET, "SYSTEM_WDT_RESET"},
        {SYSTEM_05_REG_RESET, "SYSTEM_05_REG_RESET"},
        {SYSTEM_ERROR_EYESAFETY, "SYSTEM_ERROR_EYESAFETY"},
        {SYSTEM_ERROR_OTP_CPA_CHECKFAILED, "SYSTEM_ERROR_OTP_CPA_CHECKFAILED"},
        {SYSTEM_SMOKE_STAGE1_ERROR, "SYSTEM_SMOKE_STAGE1_ERROR"},
    };

    for (size_t i = 0; i < sizeof(error_table) / sizeof(error_table[0]); i++) {
        if (DTOF_BIT_GET(error_code, error_table[i].bit)) {
            dtof_printf("%s\n", error_table[i].msg);
        }
    }
}
// extern device_driver_ops_t device_iic_driver_ops;

void dtof_reg_test() {
    // 测试寄存器读取，期望结果：0xdeaf
    // printf("=> dtof_reg_test Deprecated.\n");
    printf("=> Calling dtof_reg_test().\n");
    uint16_t input_buf[1] = {0x17b9};
    // uint16_t input_buf[1] = {0xb917};
    dtof_reg_burst_write( 0x05, input_buf, 1);
    uint16_t output_buf[1] = {0};
    dtof_reg_burst_read( 0x00, output_buf, 1);
}

void file_io_test(dtof_uint8_t test_uuid) {
    // dtof_uint8_t test_uuid = 0x0a;
    dtof_int32_t read_b;
    dtof_int32_t read_xtalk[18];
    dtof_get_distance_offset_from_flash(test_uuid, &read_b);
    DTOF_LOG("file_io_test(): read_b = %d \n", read_b);
    dtof_get_xtalk_data_from_flash(test_uuid, (uint16_t *)read_xtalk);
    DTOF_LOG(
        "file_io_test(): read_xtalk[:4] = %d, %d, %d, %d \n", 
        read_xtalk[0], read_xtalk[1], read_xtalk[2], read_xtalk[3]
    );

    dtof_int32_t write_b = -1;
    dtof_int32_t write_xtalk[] = {1, -2, 3, -4, 5, -6, 7, -8, 9, -10, 11, -12, 13, -14, 15, -16, 17, -18};
    int ret;
    ret = dtof_set_distance_offset_to_flash(test_uuid, write_b);
    // DTOF_LOG("file_io_test(): write_b ret = %d \n", ret);
    ret = dtof_set_xtalk_data_from_flash(test_uuid, (uint16_t *)write_xtalk, 0 ,0);
    // DTOF_LOG("file_io_test(): write_xtalk ret = %d \n", ret);
}

#define DTOF_SINGLE_MAIN_HISTGRAM_LEN      512 // 64 * 8, stm32/dev/dtof_hal.h
typedef int (*dtof_data_dump_func_t)(const int, const char *, const size_t);
/**
 * @brief 将hist_p的数据整理为字符串输出至串口（通过dev_handle和dump_func函数输出）
 * @param dev_handle 设备的id(用于dump_func函数)
 * @param dump_func 通信函数，int dump_func(int dev_handle, char *msg, size_t msg_len)
 * @param hist_p 数据内容（要求一定是 uint16 的数据）
 * @param len 数据长度
 */
void dump_hist_log(int dev_handle, dtof_data_dump_func_t dump_func, dtof_uint16_t* hist_p, dtof_uint16_t len){
    // printf("=> dump_hist_log() NotImplemented");
    #define OUT_PUT_MAX_BUFFER  (4096)
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
            // stm32_uart_write(0, output_str, total_used_len);
            dump_func(dev_handle, output_str, total_used_len);
            // reset the value
			temp_start = output_str;
		    total_used_len = 0;
        }
    }

    if(total_used_len != 0)
    {
        // stm32_uart_write(0, output_str, total_used_len);
        dump_func(dev_handle, output_str, total_used_len);
    }
    // stm32_uart_write(0, "\n", 1);
    dump_func(dev_handle, "\n", 1);
}

#define COMPILE_I2C_CMDS

// 主循环接收命令
void main_cmd_loop(int serial){
    static dtof_uint16_t is_to_sky_flag = 1;
    dtof_bool_t first_new_flag = DTOF_TRUE;
    DTOF_RET ret;
    dtof_uint16_t chip_id;
    dtof_uint8_t device_id = 0;
  dtof_uint16_t frame_id;
    dtof_uint16_t buffer[DTOF_SINGLE_MAIN_HISTGRAM_LEN + 64];
    dtof_distance_result_t distance_result;
    dtof_bool_t is_new_flag=DTOF_FALSE;
    dtof_bool_t is_init = DTOF_FALSE;
    dtof_bool_t debug_flag = DTOF_FALSE;
    dtof_uint16_t reg80;
    dtof_uint32_t status = 0;
    uint8_t byte;
    char cmd_buffer[CMD_BUFFER_SIZE] = {0};
    uint8_t uart_index = 0;

    dtof_bool_t frame_cnt_flag = DTOF_FALSE;
    int32_t frame_cnt = 0;
    int received = 0;

    while(1) {
        if (get_and_clear_data_ready()){
            received = rpi_serial_receive(serial, cmd_buffer, sizeof(cmd_buffer));
        }
        if (received > 0) {
            // 输出接收到的命令
#ifdef DEBUG_LOG_FLAG
            printf("=> DEBUG MSG:\n");
            printf("Received[%d bytes]: %s\n", received, cmd_buffer);
            for (size_t i = 0; i < CMD_BUFFER_SIZE; i++)
            {
                printf("  i = %d: %d,\n", i, cmd_buffer[i]);
                if (cmd_buffer[i] == 0) break;
            }
            printf("=> END\n");

            printf("=> receive cmd: %s\n", cmd_buffer);
#endif

            // 解析命令并执行
            if (strcmp(cmd_buffer, "echo") == 0) {
                // 复读串口发送的echo
                rpi_serial_printf(serial,"Receive command: %s\n", cmd_buffer);
                // rpi_serial_send(serial, cmd_buffer, received);
            }
#ifdef COMPILE_I2C_CMDS
            else if (strcmp(cmd_buffer, "s") == 0)
            {   
                printf("togos");
                // printf("进入s");
                // 启动并开始测距（无输出）
            //    DTOF_CHECK_WARN(dtof_init_and_wait_for_ready(device_id, &chip_id), "dtof init and wait for ready failed\n");
            //    DTOF_CHECK_WARN(dtof_sensor_init(), "dtof sensor init failed\n");
               
                is_init = DTOF_TRUE;
                dtof_start_distance_measure();
                debug_flag = DTOF_FALSE;
            }
            else if (strcmp(cmd_buffer, "d") == 0)
            {
                // 启动并开始测距（DEBUG模式，输出每一帧的数据，不会自动停止）
                //DTOF_CHECK_WARN(dtof_init_and_wait_for_ready(device_id, &chip_id), "dtof init and wait for ready failed\n");
                DTOF_CHECK_WARN(dtof_sensor_init(), "dtof sensor init failed\n");
                is_init = DTOF_TRUE;
                dtof_reg_burst_read(80, &reg80, 1);
                dtof_start_distance_measure();
                if (dtof_get_chip_config()->chip_id == DTOF_A05_CHIPID)
                    {
                        DTOF_CHECK_WARN(dtof_io_interaction(DTOF_CMD_WRITE_REG_ADDR, SPECIAL_BYPASS_VALUE), "enable debug mode failed\n");
                    }
                debug_flag = DTOF_TRUE;
            }
            else if (strcmp(cmd_buffer, "e") == 0)
            {
                // 启动并开始测距（DEBUG模式 + 启动frame_cnt, 前50帧跳过， 到达200帧自动停止）
              //  DTOF_CHECK_WARN(dtof_init_and_wait_for_ready(device_id, &chip_id), "dtof init and wait for ready failed\n");
             DTOF_CHECK_WARN(dtof_sensor_init(), "dtof sensor init failed\n");
              is_init = DTOF_TRUE;
              dtof_reg_burst_read(80, &reg80, 1);
                dtof_start_distance_measure();
                if (dtof_get_chip_config()->chip_id == DTOF_A05_CHIPID)
                    {
                        DTOF_CHECK_WARN(dtof_io_interaction(DTOF_CMD_WRITE_REG_ADDR, SPECIAL_BYPASS_VALUE), "enable debug mode failed\n");
                    }
                frame_cnt_flag = DTOF_TRUE;
                debug_flag = DTOF_TRUE;
            }
            else if (strcmp(cmd_buffer, "t") == 0)
            {
                printf("gotot");
            STOP_DISTANCE_MEASURE:
                // 停止测距
                dtof_stop_distance_measure();
                if(debug_flag == DTOF_TRUE)
                {
                if (dtof_get_chip_config()->chip_id == DTOF_A05_CHIPID)
                    {
                        DTOF_CHECK_WARN(dtof_io_interaction(DTOF_CMD_WRITE_REG_ADDR, SPECIAL_BYPASS_VALUE), "enable debug mode failed\n");
                    }
                }
                
            }
            else if (strncmp(cmd_buffer, "ri,", 3) == 0)
                {
                    int reg_addr = atoi(&cmd_buffer[3]);
                    uint16_t reg_value;
                    dtof_read_reg_running(reg_addr, &reg_value);
                    printf("reg read 0x%x: 0x%4x\n", reg_addr, reg_value);
                    fflush(stdout);
                }
            else if (strncmp(cmd_buffer, "rb,", 3) == 0)
            {
                // "rb,<reg_addr:int>,<reg_num:int>", 批量读地址为<reg_addr>的寄存器中，长度为<reg_num>的值
                // 输出至树莓派终端（不是串口）
                DTOF_CHECK_RET(dtof_set_mcu_status( DTOF_MCU_STATE_SLEEP_DIRECT), "set mcu sleep failed\n");
                int reg_addr, reg_num;
                dtof_uint16_t reg_max[255];
                if (sscanf(cmd_buffer, "rb,%d,%d", &reg_addr, &reg_num) == 2)
                {
                    dtof_reg_burst_read( reg_addr, reg_max, reg_num);
                    rpi_serial_printf(serial,"burst reg read 0x%04x:\n", reg_addr);
                    
                    for (int i = 0; i < reg_num; i++)
                    {
                        rpi_serial_printf(serial,"%d, ", reg_max[i]);
                        
                    }
                    rpi_serial_printf(serial,"burst reg read 0x%04x end.\n", reg_addr);
                }
                DTOF_CHECK_RET(dtof_set_mcu_status( DTOF_MCU_STATE_WAKEUP), "wakeup mcu failed\n");
            }
            else if (strncmp(cmd_buffer, "wi,", 3) == 0)
                {
                    int reg_addr, reg_value;
                    if (sscanf(cmd_buffer, "wi,%d,%d", &reg_addr, &reg_value) == 2)
                    {
                        if (reg_addr == 300){
                            is_to_sky_flag = reg_addr;
                            printf("reg write 0x%x: 0x%04x, set 300->1 for let cg cal to sky \n", reg_addr, reg_value);
                        }else{
                            dtof_write_reg_running(reg_addr, reg_value); // 示例：写入值为索引 i，你可根据实际需求改成 uart_buf 中解析的值
                            printf("reg write 0x%x: 0x%04x\n", reg_addr, reg_value);
                            fflush(stdout);
                        }
                    }
                }
            else if (strcmp(cmd_buffer, "p") == 0)
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
                    dtof_run_mode_e running_mode;
                    dtof_bool_t is_legal_data = DTOF_FALSE;
                    DTOF_CHECK_RET_VOID(dtof_read_running_mode(&running_mode), "read running mode failed\n");
                    dtof_get_ft_data_from_flash_multi_mode((dtof_uint16_t*)&ft_data_read, sizeof(dtof_ft_data_t)/sizeof(dtof_uint16_t), running_mode, &is_legal_data);
                    if (is_legal_data == DTOF_FALSE)
                    {
                        printf("ft data is illegal, all 0xFF\n");
                    }
                    else
                    {
                        #ifdef DTOF_FT_CALIBRATE_BINOFFSET
                        printf("bin_offset = %u\n", ft_data_read.dtof_ft_data.bin_offset);
                        #endif
                        #ifdef DTOF_FT_CALIBRATE_REFSPAD
                        printf("ref_spad = %u\n", ft_data_read.dtof_ft_data.ref_spad);
                        #endif
                        #ifdef DTOF_FT_CALIBRATE_CG
                        printf("cg_reg: ");
                        dtof_uint16_t cg_reg;
                        for (int i = 0; i < (DTOF_AC_NUM + 1); i++)
                        {
                            cg_reg = ft_data_read.dtof_ft_data.cg_data[i * 2 + 1] * 256 + ft_data_read.dtof_ft_data.cg_data[i * 2];
                            printf("%u, ", cg_reg);
                        }
                        printf("\n");
                        #endif
                        #ifdef DTOF_FT_CALIBRATE_B
                        printf("distance_k=%d, distance_b=%d\n", ft_data_read.dtof_ft_data.distance_k, ft_data_read.dtof_ft_data.distance_b);
                        #endif
                        }
                    parse_inner_mcu_error_code(status);
                }
            else if (strcmp(cmd_buffer, "clear") == 0)
            {
                // 清除flash（flash单点写入时只能从1置0，因此写入数据必须先置1）
                DTOF_LOG("running on raspi, stm32_flash_write_init() is deprecated.");
                // stm32_flash_write_init(DTOF_B_DATA_FLASH_PAGE, DTOF_B_DATA_FLASH_PAGE_NUM); // deprecated
                // stm32_flash_write_init(DTOF_CG_DATA_FLASH_PAGE, DTOF_CG_DATA_FLASH_PAGE_NUM); // deprecated
            }

            else if (strncmp(cmd_buffer, "ft,", 3) == 0)
                {
                    dtof_uint16_t ft_cali_type;
                    dtof_uint16_t ft_actual_param;

                    if (sscanf(cmd_buffer, "ft,%hu,%hu", &ft_cali_type, &ft_actual_param) == 2)
                    {
                        dtof_printf("start ft calibration, type=0x%04x\n", ft_cali_type);
                        DTOF_RET ret = dtof_do_ft_calibration_all_mode(ft_cali_type, ft_actual_param);
                        if (ret == DTOF_RET_SUCCESS) {
                            dtof_ft_cali_param_t ft_cali_param;
                            dtof_bool_t is_legal_data = DTOF_FALSE;
                            dtof_set_ft_data_to_flash_multi_mode((dtof_uint16_t*)&ft_cali_param, sizeof(dtof_ft_cali_param_t)/sizeof(dtof_uint16_t), &is_legal_data);
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
            else if (strcmp(cmd_buffer, "x") == 0)
            {
                // 写入串扰数据并原样输出ram？
                DTOF_CHECK_RET(dtof_set_mcu_status( DTOF_MCU_STATE_SLEEP_DIRECT), "set mcu sleep failed\n");
                #define READ_LEN 1126
                dtof_uint16_t ram_start = 0x2000;
                dtof_uint16_t ram_read[READ_LEN];
                DTOF_CHECK_RET(dtof_reg_burst_write(0XFE, &ram_start, 1),
                                "写入串扰数据失败");
                DTOF_CHECK_RET(dtof_reg_burst_read(0xff, ram_read, READ_LEN),
                                "读取距离结果失败");

                rpi_serial_printf(serial,"ramdata\n");
                for(int i = 0; i < READ_LEN; i++)
                {
                    rpi_serial_printf(serial,"0x%04x, ", ram_read[i]);
                    if ((i + 1) % 16 == 0){
                        rpi_serial_printf(serial,"\n");
                    }
                }
                rpi_serial_printf(serial,"\n");

                DTOF_CHECK_RET(dtof_set_mcu_status( DTOF_MCU_STATE_WAKEUP), "wakeup mcu failed\n");
            }
            else if (strcmp(cmd_buffer, "v") == 0)
            {
                // 输出版本信息
                rpi_serial_printf(serial,"sdk version: %s\n",dtof_get_sdk_version());
                rpi_serial_printf(serial,"chip version: %d\n", DTOF_SWAP16(dtof_get_chip_version()));
                // DTOF_LOG("soc commit: %s\n", GIT_COMMIT_HASH);
            }
#endif
            else if (strcmp(cmd_buffer, "init") == 0)
                {
                    dtof_init_device_info();
                    DTOF_CHECK_WARN(dtof_sensor_init(), "dtof sensor init failed\n");
                }
                else if (strcmp(cmd_buffer, "uuid") == 0)
                {
                    dtof_uint8_t chip_uuid_buffer[DTOF_UUID_LENGTH];
                    DTOF_CHECK_WARN(dtof_get_uuid(chip_uuid_buffer, DTOF_UUID_LENGTH), "get uuid failed\n");
                    printf("chip uuid: ");
                    for (int i = 0; i < DTOF_UUID_LENGTH; i++)
                    {
                        printf("%d, ", chip_uuid_buffer[i]);
                        fflush(stdout);
                    }
                }
                else if (strcmp(cmd_buffer, "error_code") == 0)
                {
                    dtof_uint32_t error_code;
                    DTOF_CHECK_WARN(dtof_get_error_info(&error_code), "get error info failed\n");
                    printf("error code: 0x%x\n", error_code);
                }
                else if (strncmp(cmd_buffer, "osc_cal,", 8) == 0)
                {
                    int osc_cal_mode;
                    if (sscanf(cmd_buffer, "osc_cal,%d", &osc_cal_mode) == 1)
                    {
                        DTOF_CHECK_WARN(dtof_write_reg_running(DTOF_FRAME_CONTROL_REG, (osc_cal_mode << 12) | 0x0388), "dtof start failed\n");

                    }
                }
            


            else if (strncmp(cmd_buffer, "u,", 2) == 0) {
                // 更新当前程序，（退出c程序，调用python脚本）
                char version_name[256];
                char command[512];
                
                // 安全地解析版本名称
                if (sscanf(cmd_buffer + 2, "%255s", version_name) == 1) {
                    // 构建Python命令
                    snprintf(command, sizeof(command), "python ./script/update.py download -v %s", version_name);
                    
                    // 执行Python更新脚本
                    int ret = system(command);
                    if (ret != 0) {
                        DTOF_LOG("更新脚本执行失败，返回值: %d\n", ret);
                    }
                    
                    // 退出当前C程序
                    DTOF_LOG("正在退出程序以完成更新...\n");
                    break;
                    // exit(EXIT_SUCCESS);
                } else {
                    DTOF_LOG("无效的版本名称格式\n");
                }
            }
            else if (strcmp(cmd_buffer, "q") == 0) {
                // 退出命令循环
                // printf("Quit.\n");
                break;
            }
            else {
                printf("Unknown command: %s\n", cmd_buffer);
            }

            // on_cmd_done 每次执行完命令执行
            received = 0;
        }

        // on_loop_step_done 每次循环执行
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
            ret = dtof_get_distance_result(&distance_result);
            
            frame_id=distance_result.frame_id;

            if(dtof_device_info.first_frame)
            {
                dtof_device_info.first_frame=0;
                dtof_device_info.frame_id_pre=frame_id;
                
            }
            else if (frame_id == dtof_device_info.frame_id_pre);
            

           else if (((frame_id - dtof_device_info.frame_id_pre) != 1) && ((frame_id - dtof_device_info.frame_id_pre) != 2)) {
            // 异常帧
            dtof_device_info.first_frame = 1;
           
               }
            
           else
           {
           is_new_flag = DTOF_TRUE;
             dtof_device_info.frame_id_pre = frame_id;
           }
          
            
#endif
        }  
#ifdef DTOF_POLLING_MODE
        if (dtof_get_chip_config()->chip_id== DTOF_A05_CHIPID)
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
                    if (frame_cnt < 50)
                    {
                        if (dtof_get_chip_config()->chip_id == DTOF_A05_CHIPID)
                        {
                            dtof_io_interaction(0x30, 0x01);
                            DTOF_CHECK_WARN(dtof_io_interaction(DTOF_CMD_WRITE_REG_ADDR, SPECIAL_BYPASS_VALUE), "enable debug mode failed\n");
                        }
                        goto PASS;
                    }
                }
                if (debug_flag == DTOF_TRUE)
                {
                    DTOF_CHECK_WARN(dtof_debug_mode_bypass(dtof_get_chip_config()->chip_id), "debug mode bypass failed\n");
                }

#define TOTAL_REG_NUM 255
                dtof_histgram_io_read(DTOF_SINGLE_MAIN_HISTGRAM_OFFSET, buffer, DTOF_SINGLE_MAIN_HISTGRAM_LEN);
                dump_hist_log(serial, rpi_serial_send,buffer, DTOF_SINGLE_MAIN_HISTGRAM_LEN);
                dtof_histgram_io_read(DTOF_SINGLE_REF_HISTGRAM_OFFSET, buffer, DTOF_SINGLE_REF_HISTGRAM_LEN);
                dump_hist_log(serial, rpi_serial_send, buffer, DTOF_SINGLE_REF_HISTGRAM_LEN);
                dtof_dsp_fifo_read(0, buffer, DTOF_SINGLE_FIFO_LEN);
                dump_hist_log(serial, rpi_serial_send, buffer, DTOF_SINGLE_FIFO_LEN);
                dtof_reg_burst_read(0x00, buffer, TOTAL_REG_NUM);
                dump_hist_log(serial, rpi_serial_send, buffer, TOTAL_REG_NUM);

                dtof_set_mcu_status(DTOF_MCU_STATE_WAKEUP);

                if (dtof_get_chip_config()->chip_id == DTOF_A05_CHIPID)
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
                        rpi_serial_printf(serial,"%d, %d, %d, %d, %.6f, %d, %.6f, %.6f, %.6f, %.6f, %d, %f, %f, %f, %d\n",
                            distance_result.frame_id, distance_result.first_target, distance_result.first_intensity, distance_result.main_nflash, distance_result.ambient, distance_result.is_legal_frame,
                            distance_result.main_peak_pos, distance_result.second_peak_pos, distance_result.ref_peak_pos, distance_result.ref_peak_pos_smooth,
                            distance_result.ref_peak_hist, distance_result.first_target_raw, distance_result.reflect_compensation, distance_result.ambient_compensation, distance_result.is_swap_peak);;
                        is_new_flag = DTOF_FALSE;
                        goto STOP_DISTANCE_MEASURE;
                    }
                }
            }
            
             rpi_serial_printf(serial,"%d, %d, %d, %d, %.6f, %d, %.6f, %.6f, %.6f, %.6f, %d, %f, %f, %f, %d\n",
                            distance_result.frame_id, distance_result.first_target, distance_result.first_intensity, distance_result.main_nflash, distance_result.ambient, distance_result.is_legal_frame,
                            distance_result.main_peak_pos, distance_result.second_peak_pos, distance_result.ref_peak_pos, distance_result.ref_peak_pos_smooth,
                            distance_result.ref_peak_hist, distance_result.first_target_raw, distance_result.reflect_compensation, distance_result.ambient_compensation, distance_result.is_swap_peak);
            is_new_flag = DTOF_FALSE;
        }
        PASS:
        {
            
        }  
    }
}


int main() {
    // TODO: 编译前 需要先查一下当前的 arm 是 32 还是 64的，然后将makefile中的 lds 做修改，指定为是 64的 或 32的
    int ret;
    
    extern int rpi_gpio_init(void);
  
    ret = rpi_gpio_init();
    if(ret){
        printf("Failed to init rpi gpio");
        return 1;
    }

    rpi_i2c_init(1);
    
    // 初始化串口
    int serial = rpi_serial_init(SERIAL_PORT);

   DTOF_CHECK_WARN(dtof_sensor_init(), "dtof sensor init failed\n");
    
    printf("serial init...\n");

    // 主循环接收命令
    main_cmd_loop(serial);

    // 清理资源
    rpi_gpio_cleanup();

    // printf("=> Safely exited.\n");
    return 0;
}
