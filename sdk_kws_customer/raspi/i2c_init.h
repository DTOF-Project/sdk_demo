#ifndef I2C_INIT_H
#define I2C_INIT_H

#include <stdint.h>

// I2C 总线设备文件
#define I2C_BUS "/dev/i2c-1"
// I2C 从设备地址
#define DEVICE_ADDR 0x41

// static int rpi_i2c_read_block(int iic_id, uint8_t reg, uint8_t *output_buf, uint16_t read_len);

#endif // I2C_INIT_H