#ifndef I2C_INIT_H
#define I2C_INIT_H

// #include <stdint.h>
#include "../sdk/inc/dtof_base_type.h"

// I2C 总线设备文件
#define I2C_BUS "/dev/i2c-1"
// I2C 从设备地址
#define DEVICE_ADDR 0x41
//DTOF_RET rpi_i2c_init(int iic_id);
typedef struct {
    DTOF_RET (*init)(int device_id);
    DTOF_RET (*deinit)(int device_id);
    DTOF_RET (*read)(int device_id, void *buf, int nbyte);
    DTOF_RET (*write)(int device_id, void *buf, int nbyte);
    DTOF_RET (*write_word)(int device_id, uint8_t reg, const uint16_t val);
    DTOF_RET (*write_block)(int device_id, uint8_t reg, uint8_t *input_buf, uint16_t input_len);
    DTOF_RET (*read_word)(int device_id, uint8_t reg, uint16_t *buf);
    DTOF_RET (*read_block)(int device_id, uint8_t reg, uint8_t *output_buf, uint16_t read_len);
    DTOF_RET (*configure)(int device_id, void *cfg);
    DTOF_RET (*get_attribute)(int device_id, int attribute, void *attr, uint32_t *attrlen);
    DTOF_RET (*set_attribute)(int device_id, int attribute, void *attr, uint32_t attrLen);
    DTOF_RET (*ioctl)(int device_id, int cmd, void *arg);
} device_driver_ops_t; // stm32/user/device/device.h

#endif // I2C_INIT_H