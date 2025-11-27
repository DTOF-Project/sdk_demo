#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <time.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <signal.h>
#include <string.h>

#include "inc/dtof_common.h"
#include "inc/dtof_libc.h"
#include "inc/dtof_endian.h"
#include "inc/dtof_api.h"
#include "raspi/i2c_init.h"
#include "stm32/user/device/ds_sal.h"
// #include "app_new/cmd/inc/app_cmd.h"
#include "signal_manager.h"
#include "dtof_interrupt_handler.h"
//#include "command_receiver.h"    // 更新为新的头文件名
#include <cJSON.h>  // 添加在文件开头的其他include语句之后
#include <openssl/buffer.h>  // Add this for BUF_MEM
#include <openssl/bio.h>
#include <openssl/evp.h>
#include "gpio_init.h"
#include "stm32/user/device/device.h"
static ds_sal_config_t raps_peripheral;
static int board_peripheral()
{
    // 这里用到的 是 i2c1
    #define IIC_NBR0 1
    raps_peripheral.common_cfg.comm_type = COMM_IIC;
    raps_peripheral.common_cfg.comm_channel_id = IIC_NBR0;
    return 0;
}
extern device_driver_ops_t device_iic_driver_ops;
// static int data_socket = -1;

// 初始化与Rust后端的通信
// static int init_socket_comm(void) {
//     struct sockaddr_un addr;
//     #define SOCKET_PATH "/tmp/dtof_data.sock"
//     data_socket = socket(AF_UNIX, SOCK_DGRAM, 0);
//     if (data_socket == -1) {
//         perror("socket创建失败");
//         return -1;
//     }

//     memset(&addr, 0, sizeof(addr));
//     addr.sun_family = AF_UNIX;
//     strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path)-1);

//     if (connect(data_socket, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
//         perror("socket连接失败");
//         close(data_socket);
//         data_socket = -1;
//         return -1;
//     }

//     return 0;
// }

void signal_handler(int sig) {
    printf("接收到信号 %d,准备退出...\n", sig);
    signal_manager_notify_exit();
}
// Base64编码函数
// static char* base64_encode(const uint8_t* input, size_t length) {
//     BIO *bio, *b64;
//     BUF_MEM *bufferPtr;
//     char* encoded;

//     b64 = BIO_new(BIO_f_base64());
//     bio = BIO_new(BIO_s_mem());
//     bio = BIO_push(b64, bio);

//     BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
//     BIO_write(bio, input, length);
//     BIO_flush(bio);
//     BIO_get_mem_ptr(bio, &bufferPtr);

//     encoded = (char*)malloc(bufferPtr->length + 1);
//     memcpy(encoded, bufferPtr->data, bufferPtr->length);
//     encoded[bufferPtr->length] = '\0';

//     BIO_free_all(bio);
//     return encoded;
// }
// static void process_queue_data(void) {
//     dtof_raw_data_t raw_data;
//     char *dis_res, *reg_fifo;
//     // *dsp_fifo, *main_hist, *ref_hist;
//     //  *other_data;
//     // 从队列中获取数据
//     extern int dtof_queue_pop_timeout(dtof_raw_data_t *data, long timeout_ms);
//     if (dtof_queue_pop_timeout(&raw_data,100) == 0) {
//         dis_res = uint16_array_to_hex_string((uint16_t *)&raw_data.result_info, sizeof(raw_data.result_info)/sizeof(uint16_t));
//         reg_fifo = uint16_array_to_hex_string((uint16_t *)&raw_data.reg_fifo, sizeof(raw_data.reg_fifo)/sizeof(uint16_t));

//         // 创建JSON对象并发送
//         cJSON *root = cJSON_CreateObject();
//         cJSON_AddStringToObject(root, "cmd", "trap");
//         cJSON_AddNumberToObject(root, "time", raw_data.milliseconds);
//         cJSON_AddNumberToObject(root, "frame_index", raw_data.frame_index);
//         cJSON_AddStringToObject(root, "dis_result", dis_res);
//         cJSON_AddStringToObject(root, "reg_fifo", reg_fifo);
//         // cJSON_AddStringToObject(root, "fifo_dis", raw_data.fifo_buf);
//         // cJSON_AddStringToObject(root, "mainhist", raw_data.main_hist);
//         // cJSON_AddStringToObject(root, "refhist", raw_data.ref_hist);
//         // cJSON_AddStringToObject(root, "fifo_raw", raw_data.raw_data);

//         // char *json_str = cJSON_PrintUnformatted(root);
//         // printf("%s\n", json_str);

//         send_response(get_sockfd(), get_client_addr(), 0, (char*)root);

//         // 释放资源
//         free(dis_res);
//         // free(dsp_fifo);
//         // free(main_hist);
//         // free(ref_hist);
//         // free(other_data);
//         cJSON_Delete(root);
//         // free(json_str);
//     }
// }

int main() {
    int ret;
    board_peripheral();
    //app_cmd_init();
    signal_manager_init();

    extern int rpi_gpio_init(void);
    printf("rpi_gpio_init...\n");
    ret = rpi_gpio_init();
    if(ret){
        printf("Failed to init rpi gpio");
        return 1;
    }

    
    rpi_i2c_init(1);

    // 测试寄存器读取，期望结果：0xdeaf
    uint8_t testReadResult[2];
    device_iic_driver_ops.read_block(1, DEVICE_ADDR, testReadResult, 2);
    printf("=> testReadResult: %x, %x \n", testReadResult[0], testReadResult[1]);

// extern DTOF_RET ds_device_peripheral_init(ds_sal_config_t *peripheralConfig);
//     ds_device_peripheral_init(&raps_peripheral);

//     ret = command_receiver_start();
//     if (ret != 0) {
//         signal_manager_notify_exit();
//         goto ret_flag;
//     }


    // // 创建 DTOF 中断处理线程
    // ret = dtof_interrupt_handler_start();
    // if (ret != DTOF_HANDLER_OK) {
    //     printf("DTOF 中断处理线程创建失败: %d\n", ret);
    //     signal_manager_notify_exit();
    //     goto ret_flag;
    // }

    // ret = dtof_init();
    // if (ret != 0) {
    //     printf("dtof init fail\n");
    //     signal_manager_notify_exit();
    //     goto ret_flag;
    // }

    // while (signal_manager_should_continue()) {
    //     process_queue_data();
    //}

//ret_flag:

    // printf("正在清理资源...\n");
    // command_receiver_wait();
    // dtof_interrupt_handler_wait();

    // 清理资源
    rpi_gpio_cleanup();

    printf("应用程序已安全退出\n");
    return 0;
}
