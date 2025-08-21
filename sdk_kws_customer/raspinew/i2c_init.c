#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <linux/i2c.h>        // 添加这个头文件
#include <linux/i2c-dev.h>    // 添加这个头文件
#include <sys/ioctl.h>
#include <unistd.h>
#include <errno.h>
#include "i2c_init.h"
#include "platform_user_config.h"    // 添加平台配置头文件
#include "sdk/inc/util.h"

int dtof_file;  // 定义全局变量

DTOF_RET rpi_i2c_init(int iic_id) {
    // 打开 I2C 设备文件
    if ((dtof_file = open(I2C_BUS, O_RDWR)) < 0) {
        perror("Failed to open the i2c bus");
        return DTOF_RET_ERROR;
    }

    // 设置从设备地址
    if (ioctl(dtof_file, I2C_SLAVE, DEVICE_ADDR) < 0) {
        perror("Failed to set the iic slave address");
        close(dtof_file);
        return DTOF_RET_ERROR;
    }

    

    return DTOF_SUCCESS;
}

DTOF_RET  rpi_i2c_deinit(int iic_id) {
    if (dtof_file >= 0) {
        close(dtof_file);
        return DTOF_SUCCESS;
    }
    return DTOF_RET_ERROR;
}
static int rpi_i2c_write_block(int iic_id, uint8_t reg, uint8_t *input_buf, uint16_t input_len) {
    uint16_t write_len;

    if (!input_buf || input_len == 0) {
        return DTOF_RET_ERROR;
    }

    write_len = input_len * 2;

    // 分配临时缓冲区，大小为寄存器地址(1字节) + 数据长度
    uint8_t *write_buf = (uint8_t *)malloc(write_len + 1);
    if (!write_buf) {
        return DTOF_RET_ERROR;
    }

    // 组装数据：寄存器地址 + 数据
    write_buf[0] = reg;
    memcpy(write_buf + 1, input_buf, write_len);

    // 执行写入操作
    int ret = write(dtof_file, write_buf, write_len + 1);

    // 释放临时缓冲区
    free(write_buf);

    // 检查写入结果
    if (ret != write_len + 1) {
        return DTOF_RET_ERROR;
    }

    return DTOF_RET_SUCCESS;
}

static int rpi_i2c_read_block(int iic_id, uint8_t reg, uint8_t *output_buf, uint16_t read_len) {
    if (!output_buf || read_len == 0) {
        return DTOF_RET_ERROR;
    }

    struct i2c_msg messages[2];
    struct i2c_rdwr_ioctl_data packets;

    // 配置写消息（寄存器地址）
    messages[0].addr = DEVICE_ADDR;
    messages[0].flags = 0;        // 写操作
    messages[0].len = 1;
    messages[0].buf = &reg;

    // 配置读消息（数据读取）
    messages[1].addr = DEVICE_ADDR;
    messages[1].flags = I2C_M_RD; // 读操作
    messages[1].len = read_len * 2;
    messages[1].buf = output_buf;

    // 组合消息
    packets.msgs = messages;
    packets.nmsgs = 2;

    // 执行组合读写操作
    if (ioctl(dtof_file, I2C_RDWR, &packets) < 0) {
        return DTOF_RET_ERROR;
    }

    return DTOF_RET_SUCCESS;
}


device_driver_ops_t device_iic_driver_ops = {
    .init = rpi_i2c_init,
    .deinit = rpi_i2c_deinit,
    .write_block = rpi_i2c_write_block,
    .read_block = rpi_i2c_read_block,
   // .ioctl = ioctl,
};
