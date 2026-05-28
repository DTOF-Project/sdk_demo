/*
 * main.c
 *
 *  Created on: 2024/9/11
 *      Author: liuzihao
 */

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
#include "device/gpio_init.h"
#include "serial_init.h"

#include "main.h"
#include "inc/dtof_api.h"
#include "inc/dtof_log.h"
#include "inc/dtof_driver.h"
#include "inc/dtof_endian.h"
#include "inc/dtof_calibration_ft.h"
#include "inc/dtof_global_config.h"
#include "inc/dev/dtof_dev_api.h"
#include "data_base/sensor_database.h"
#include "customer/dtof_customer.h"

#include "inc/dev/dtof_hal.h"

#include "user/device/ds_sal.h"
#include "user/device/ds_dev.h"
#include "user/device/device.h"

#include "application/inc/soc_version.h"
#include "application/inc/app_cmd.h"
#include "application/inc/app_distance.h"

dtof_uint16_t g_frame_id;

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


    DTOF_CHECK_WARN(dtof_peripheral_device_init(), "dtof_peripheral_device_init failed\n");

    DTOF_CHECK_WARN(dtof_sensor_init(), "dtof sensor init failed\n");

    DTOF_CHECK_WARN(dtof_get_uuid(dev->chip_uuid, DTOF_UUID_LENGTH), "get uuid failed\n");

    // save chip type
    DTOF_CHECK_WARN(ds_get_chip_type(dtof_get_chip_config()->chip_id, &dev->chip_type), "get chip type failed\n");

    uint8_t byte;

    dtof_set_interrupt_flag(DTOF_FALSE);

    DTOF_CHECK_RET(dtof_reg_burst_read(DTOF_SAVE_RESULT_REG_ADDR, &g_frame_id, 1), "read frame id failed\n");

    while (1)
    {
        int len = dev->device_uart_driver->read(0, &byte, 1);

        if (len == 1)
        {
            parse_cmd_process(byte);
        }

        app_distance_process();
    }

    return 0;
}
