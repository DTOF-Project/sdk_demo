/*
 * ds_dev.h
 *
 *  Created on: 2024/9/11
 *      Author: liuzihao
 */
#ifndef SRC_INC_DS_DEV_H_
#define SRC_INC_DS_DEV_H_

#include "device.h"

typedef void (*dsd_interrupt_callback)(void *param);

typedef struct dsd_interrupt_s
{
    dtof_uint32_t irq_flag;
    dsd_interrupt_callback irq_cb;
    dev_handle_t device_id; /*!< device ID.用于中断回调的参数输入 */
} dsd_interrupt_t;

typedef enum {
    DTOF_CHIP_TYPE_GNSS01,
    DTOF_CHIP_TYPE_UNKNOWN,
} dtof_chipType_t;

/**
 * @ingroup device manage
 * <p>
 * @brief device manage structure
 */
typedef struct dtof_device_s
{
#define SENSOR_F_INITED 0x01 /*!< device通信初始化bit flag. 0表示使用未初始化, 1表示已初始化 */
#define SENSOR_F_I2C 0x02    /*!< I2C通信bit flag. 0表示使用spi通信, 1表示使用I2C通信 */
#define SENSOR_F_SIMULATOR 0x04
#define SENSOR_F_LITTLEENDIAN 0x08 /*!< 小端通信bit flag. 0表示使用大端, 1表示使用小端 */
    dtof_uint32_t sensor_flags;         /*!< Flags for the this device */

    ds_sal_config_t dsd_peripheral; /*!< dtof sensor 外围接口配置 */
    dsd_interrupt_t dsd_interrupt;
    dtof_chipType_t chip_type; /*!< 芯片类型 */
    device_driver_ops_t *device_driver; // iic or spi
    device_driver_ops_t *device_uart_driver; // uart
    device_driver_gpio_ops_t *device_gpio_driver; // gpio
} dtof_device_t;

////////////////////////////////////////////////////////////////////////////////
/// @defgroup Device_Manage_Exported_Functions
/// @{

/**
 * @brief 初始化dtof外围接口
 * @param[in]  deviceID dtof device ID
 * @param[in]  peripheralConfig dtof外围接口配置
 * @return DTOF_RET
 */
DTOF_RET ds_device_peripheral_init(ds_sal_config_t *peripheralConfig);

/**
 * @brief 获取dtof device控制结构
 * @param[in] deviceID dtof device id
 * @return dtof device指针
 */
dtof_device_t *ds_device_get(void);

int dtof_reg_burst_write(uint8_t device_id, uint8_t reg_addr, uint16_t *reg_data_p, uint16_t len);

int dtof_reg_burst_read(uint8_t device_id, uint8_t reg_addr, uint16_t *reg_data_p, uint16_t len);

DTOF_RET device_driver_ops_init(dtof_device_t *dev);

#endif
