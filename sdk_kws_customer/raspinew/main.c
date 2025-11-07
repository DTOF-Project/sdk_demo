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

#include "raspinew/dtof_customer.h"
// #include "stm32/customer/dtof_customer.h"
// #include "stm32/dev/dtof_hal.c"

#define XTALK_DATA_SIZE 18
#define DO_OFFSET_CALIBRATION_MODE 2
#define DO_XTALK_CALIBRATION_MODE 1
#define CMD_BUFFER_SIZE 256

dtof_device_info_t dtof_device_info={
    .first_frame=1,
    .frame_id_pre=0,
};


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

    // uint8_t test_read_result[2];
    // device_iic_driver_ops.read_block(1, DEVICE_ADDR, test_read_result, 2);
    // printf("=> test_read_result: %x %x \n", test_read_result[0], test_read_result[1]);
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

    DTOF_RET ret;
    dtof_uint16_t chip_id;
    dtof_uint8_t device_id = 0;
  dtof_uint16_t frame_id;
    dtof_uint16_t buffer[DTOF_SINGLE_MAIN_HISTGRAM_LEN + 64];
    dtof_distance_result_t distance_result;
    dtof_bool_t is_new_flag=DTOF_FALSE;
    dtof_bool_t is_init = DTOF_FALSE;
    dtof_bool_t debug_flag = DTOF_FALSE;

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
                // 启动并开始测距（无输出）
            //    DTOF_CHECK_WARN(dtof_init_and_wait_for_ready(device_id, &chip_id), "dtof init and wait for ready failed\n");
                dtof_sensor_init();
               
                is_init = DTOF_TRUE;
                dtof_start_distance_measure();
                debug_flag = DTOF_FALSE;
            }
            else if (strcmp(cmd_buffer, "d") == 0)
            {
                // 启动并开始测距（DEBUG模式，输出每一帧的数据，不会自动停止）
                //DTOF_CHECK_WARN(dtof_init_and_wait_for_ready(device_id, &chip_id), "dtof init and wait for ready failed\n");
                dtof_sensor_init();
                is_init = DTOF_TRUE;
                dtof_start_distance_measure();
                debug_flag = DTOF_TRUE;
            }
            else if (strcmp(cmd_buffer, "e") == 0)
            {
                // 启动并开始测距（DEBUG模式 + 启动frame_cnt, 前50帧跳过， 到达200帧自动停止）
              //  DTOF_CHECK_WARN(dtof_init_and_wait_for_ready(device_id, &chip_id), "dtof init and wait for ready failed\n");
              dtof_sensor_init(); 
              is_init = DTOF_TRUE;
                dtof_start_distance_measure();
                frame_cnt_flag = DTOF_TRUE;
                debug_flag = DTOF_TRUE;
            }
            else if (strcmp(cmd_buffer, "t") == 0)
            {
                // 停止测距
                dtof_stop_distance_measure();
            }
            else if (strncmp(cmd_buffer, "c,", 2) == 0)
            {
                // "c,<value:int>", 设置b偏移为<value>
                int value = atoi(&cmd_buffer[2]);
                rpi_serial_printf(serial,"set b offset: %d\n", value);
                // stm32_flash_write_init(DTOF_B_DATA_FLASH_PAGE, DTOF_B_DATA_FLASH_PAGE_NUM); // TODO
                dtof_set_distance_offset_to_flash(device_id, value);
            }
            else if (strncmp(cmd_buffer, "r,", 2) == 0)
            {   
                // "r,<reg_addr:int>", 读地址为<reg_addr>的寄存器的值
                int reg_addr = atoi(&cmd_buffer[2]);
                uint16_t reg_value;
                
                dtof_reg_burst_read( reg_addr, &reg_value, 1);
                rpi_serial_printf(serial, "reg read 0x%x: 0x%4x\n", reg_addr, reg_value);
                // rpi_serial_printf(serial,
                // "reg read 0x%x: 0x%4x\n", reg_addr, reg_value
                // );
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
            else if (strncmp(cmd_buffer, "w,", 2) == 0)
            {
                // "w,<reg_addr:int>,<reg_value:int>" 向<reg_addr>寄存器写<reg_value>
                int reg_addr, reg_value;
                if (sscanf(cmd_buffer, "w,%d,%d", &reg_addr, &reg_value) == 2)
                {   
                    dtof_reg_burst_write( reg_addr, (uint16_t *)&reg_value, 1);
                    // dtof_write_reg_running(reg_addr, reg_value); // 示例：写入值为索引 i，你可根据实际需求改成 cmd_buffer 中解析的值
                    rpi_serial_printf(serial,"reg write 0x%x: 0x%04x\n", reg_addr, reg_value);
                }
            }
            else if (strcmp(cmd_buffer, "p") == 0)
            {
                // 输出chip uuid, distance offset, xtalk data
                dtof_uint8_t chip_uuid[DTOF_UUID_LENGTH];
                dtof_int32_t read_distance_offset = 0;
                dtof_uint16_t xtalk_data_read[XTALK_DATA_SIZE];
                DTOF_CHECK_WARN(dtof_get_uuid( chip_uuid, DTOF_UUID_LENGTH), "get uuid failed\n");
                rpi_serial_printf(serial,"chip uuid: ");
                for (int i = 0; i < DTOF_UUID_LENGTH; i++)
                {
                    rpi_serial_printf(serial,"%d, ", chip_uuid[i]); // TODO
                }
                rpi_serial_printf(serial,"\n");
                dtof_get_distance_offset_from_flash(device_id, &read_distance_offset);
                rpi_serial_printf(serial,"distance offset = %d\n", read_distance_offset);
                dtof_get_xtalk_data_from_flash(device_id, xtalk_data_read);
                rpi_serial_printf(serial,"xtalk data = ");
                for (int i = 0; i < XTALK_DATA_SIZE; i++)
                {
                    rpi_serial_printf(serial,"%d, ", xtalk_data_read[i]);
                }
                rpi_serial_printf(serial,"\n");
            }   
            else if (strcmp(cmd_buffer, "cal") == 0)
            {   
                // 输出 distance_offset
                // stm32_flash_write_init(DTOF_B_DATA_FLASH_PAGE, DTOF_B_DATA_FLASH_PAGE_NUM); // deprecated
             //   DTOF_CHECK_WARN(dtof_init_and_wait_for_ready(device_id, &chip_id, DO_OFFSET_CALIBRATION_MODE), "dtof init and wait for ready failed\n");
             dtof_sensor_init();   
             is_init = DTOF_TRUE;
               // rpi_serial_printf(serial,"distance offset = %d\n", dtof_get_distance_offset(device_id));
            }
            else if (strcmp(cmd_buffer, "clear") == 0)
            {
                // 清除flash（flash单点写入时只能从1置0，因此写入数据必须先置1）
                DTOF_LOG("running on raspi, stm32_flash_write_init() is deprecated.");
                // stm32_flash_write_init(DTOF_B_DATA_FLASH_PAGE, DTOF_B_DATA_FLASH_PAGE_NUM); // deprecated
                // stm32_flash_write_init(DTOF_CG_DATA_FLASH_PAGE, DTOF_CG_DATA_FLASH_PAGE_NUM); // deprecated
            }
            else if (strcmp(cmd_buffer, "b") == 0)
            {   
                // 从flash获取xtalk_data
                // stm32_flash_write_init(DTOF_CG_DATA_FLASH_PAGE, DTOF_CG_DATA_FLASH_PAGE_NUM); // deprecated
              //  DTOF_CHECK_WARN(dtof_init_and_wait_for_ready(device_id, &chip_id, DO_XTALK_CALIBRATION_MODE), "dtof init and wait for ready failed\n");
              dtof_sensor_init();
              uint16_t xtalk_data[18];
                dtof_get_xtalk_data_from_flash(device_id, xtalk_data);
                // is_init = DTOF_TRUE; // cg 和 b 都校准完才视为校准完成
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
            else if (strcmp(cmd_buffer, "regtest") == 0) {
                // 测试寄存器读写api
                dtof_reg_test(device_id);
            }
            else if (strcmp(cmd_buffer, "filetest") == 0) {
                // 测试文件读写api
                file_io_test(0x0a);
            }
            else if (strcmp(cmd_buffer, "tt") == 0) {
                // 测试文件读写api
                
                // DTOF_LOG("DTOF_LOG\n");
                // printf("printf\n");
                rpi_serial_printf(serial,"start test\n");
                DTOF_CHECK_RET(dtof_set_mcu_status(DTOF_MCU_STATE_SLEEP_DIRECT), "set mcu sleep failed\n");
                uint16_t reg_value;

                for (int i = 0; i<10000;i++){
                    dtof_reg_burst_read( 0, &reg_value, 1);
                    if (reg_value != 0x4120){
                        rpi_serial_printf(serial,"read error value is %x\n", reg_value);
                    }

                }
                rpi_serial_printf(serial,"end read test for 10000 times\n");
                
                
                
            }
            else if (strcmp(cmd_buffer, "sleep") == 0) {
                // 测试文件读写api
                DTOF_CHECK_RET(dtof_set_mcu_status(DTOF_MCU_STATE_SLEEP_DIRECT), "set mcu sleep failed\n");
                
            }
            else if (strcmp(cmd_buffer, "wake") == 0) {
                // 测试文件读写api
                DTOF_CHECK_RET(dtof_set_mcu_status( DTOF_MCU_STATE_WAKEUP), "set mcu sleep failed\n");
                
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
            // 检查是否有中断触发
            // rpi_gpio_debug_up_down(1);
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
          
            

        }  


        
        if (is_new_flag == DTOF_TRUE)
        {   
            is_new_flag=DTOF_FALSE;
            
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
                        dtof_stop_distance_measure();
                        DTOF_LOG("frame_cnt == 200, stopped.");
                    }
                }
                
                // bypass, read debug info
#define TOTAL_REG_NUM 255
                dtof_set_mcu_status( DTOF_MCU_STATE_SLEEP_DIRECT);

                dtof_histgram_io_read(DTOF_SINGLE_MAIN_HISTGRAM_OFFSET, buffer, DTOF_SINGLE_MAIN_HISTGRAM_LEN);
                dump_hist_log(serial, rpi_serial_send, buffer, DTOF_SINGLE_MAIN_HISTGRAM_LEN);
                dtof_histgram_io_read(DTOF_SINGLE_REF_HISTGRAM_OFFSET, buffer, DTOF_SINGLE_REF_HISTGRAM_LEN);
                dump_hist_log(serial, rpi_serial_send, buffer, DTOF_SINGLE_REF_HISTGRAM_LEN);
                dtof_dsp_fifo_read(0, buffer, DTOF_SINGLE_FIFO_LEN);
                dump_hist_log(serial, rpi_serial_send, buffer, DTOF_SINGLE_FIFO_LEN);
                dtof_reg_burst_read(0x00, buffer, TOTAL_REG_NUM);
                dump_hist_log(serial, rpi_serial_send, buffer, TOTAL_REG_NUM);

                dtof_set_mcu_status(DTOF_MCU_STATE_WAKEUP);
            }
            rpi_serial_printf(serial,
                "%d, %d, %d, %d, %.6f, %d\n",
                distance_result.frame_id, distance_result.first_target, distance_result.first_intensity, distance_result.main_nflash, distance_result.ambient, distance_result.reserved[6]
            );
            
            // rpi_gpio_debug_up_down(0);
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
    printf("serial init...\n");

    // 主循环接收命令
    main_cmd_loop(serial);

    // 清理资源
    rpi_gpio_cleanup();

    // printf("=> Safely exited.\n");
    return 0;
}
