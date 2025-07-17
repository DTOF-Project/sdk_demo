/**
 * @file device.c
 * @author liuzihao
 * @brief
 * @version 1.0.1
 * @date 2024-11-27
 *
 * @copyright Copyright (c) 2024
 *
 */

#include "inc/dtof_common.h"
#include "user/device/ds_sal.h"
#include "user/device/device.h"
#include "user/device/ds_dev.h"

extern device_driver_ops_t device_spi_driver_ops;
extern device_driver_ops_t device_iic_driver_ops;
extern device_driver_ops_t device_uart_driver_ops;
extern device_driver_gpio_ops_t device_gpio_driver_ops;

DTOF_RET device_init(dtof_uint32_t device_id)
{
	return ds_device_get()->device_driver->init(device_id);
}

DTOF_RET device_deinit(dtof_uint32_t device_id)
{
    return ds_device_get()->device_driver->deinit(device_id);
}

DTOF_RET device_read_block(dtof_uint32_t device_id, uint8_t reg, uint8_t *output_buf, uint16_t read_len)
{
    return ds_device_get()->device_driver->read_block(device_id, reg, output_buf, read_len);
}

DTOF_RET device_write_block(dtof_uint32_t device_id, uint8_t reg, uint8_t *input_buf, uint16_t input_len)
{
    return ds_device_get()->device_driver->write_block(device_id, reg, input_buf, input_len);
}

// gpio device
DTOF_RET device_gpio_init(dtof_uint32_t gpio, dtof_uint32_t cfgset)
{
    return ds_device_get()->device_gpio_driver->init_gpio_fn(gpio, cfgset);
}

DTOF_RET device_gpio_deinit(dtof_uint32_t gpio)
{
    return ds_device_get()->device_gpio_driver->deinit_gpio_fn(gpio);
}

DTOF_RET device_gpio_read(dtof_uint32_t gpio, dtof_uint32_t *value)
{
    return ds_device_get()->device_gpio_driver->read_gpio_fn(gpio, value);
}

DTOF_RET device_gpio_write(dtof_uint32_t gpio, dtof_uint32_t value)
{
    return ds_device_get()->device_gpio_driver->write_gpio_fn(gpio, value);
}

DTOF_RET device_gpio_irq_attach(dtof_uint32_t gpio, int priority, gpio_isr_fn isr, void *isr_data)
{
    return ds_device_get()->device_gpio_driver->irq_attach_fn(gpio, priority, isr, isr_data);
}

// uart device
DTOF_RET device_uart_init(dtof_int32_t device_id)
{
    return ds_device_get()->device_uart_driver->init(device_id);
}

DTOF_RET device_uart_deinit(dtof_int32_t device_id)
{
    return ds_device_get()->device_uart_driver->deinit(device_id);
}

DTOF_RET device_uart_read(dtof_int32_t device_id, void *buf, dtof_int32_t nbyte)
{
    return ds_device_get()->device_uart_driver->read(device_id, buf, nbyte);
}

DTOF_RET device_uart_write(dtof_int32_t device_id, void *buf, dtof_int32_t nbyte)
{
    return ds_device_get()->device_uart_driver->write(device_id, buf, nbyte);
}


DTOF_RET device_driver_ops_init(dtof_device_t *dev)
{
    DTOF_RET ret = DTOF_RET_SUCCESS;

    dev->device_gpio_driver = &device_gpio_driver_ops;
    dev->device_uart_driver = &device_uart_driver_ops;

    switch(dev->dsd_peripheral.common_cfg.comm_type){
        case COMM_IIC:
        {
            dev->device_driver = &device_iic_driver_ops;
            break;
        }
        case COMM_SPI:
        {
            dev->device_driver = &device_spi_driver_ops;
            break;
        }
        default:
        {
            ret = DTOF_RET_ERROR;
            break;
        }
    }
    return ret;
}