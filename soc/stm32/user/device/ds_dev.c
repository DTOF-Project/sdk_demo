/*
 * ds_dev.c
 *
 *  Created on: 2024/9/11
 *      Author: liuzihao
 */

#include "inc/dtof_base_type.h"
#include "inc/dtof_float.h"
#include "inc/dtof_common.h"
#include "inc/dtof_libc.h"
#include "inc/dtof_endian.h"
#include "inc/dtof_log.h"
#include "inc/util.h"
#include "customer/dtof_customer.h"
#include "user/device/ds_sal.h"
#include "user/device/ds_dev.h"
#include "user/device/device.h"
#include "base/inc/mos_def.h"
#include "platform_user_config.h"


// typedef DTOF_RET (*gpio_isr_fn)(int irq, void *context, void *priv);
typedef DTOF_RET (*_gpio_isr_fn)(int irq, void *context, void *priv);

extern void delay_time_us(uint64_t us);

static dtof_device_t ds_dev;

dtof_bool_t g_use_iic = DTOF_TRUE;

/***********************************************************************************/
static DTOF_RET _gpio_irq_isr(int irq, void *context, void *priv)
{
    dtof_set_interrupt_flag(DTOF_TRUE);
    return DTOF_RET_OK;
}

static DTOF_RET _ds_device_peripheral_gpio_init(dtof_device_t *dev) {
    dtof_uint32_t i = 0;
    DTOF_RET ret = DTOF_RET_SUCCESS;
    ds_sal_pin_cfg_t *gpio = dev->dsd_peripheral.pin_cfg;
    ds_sal_pin_set_t *gpio_set = dev->dsd_peripheral.pin_set;
    while (i < dssal_pin_max) {
        if (gpio[i].pin_id) {
            ret = device_gpio_init(gpio[i].pin_id, gpio[i].pin_config);
            if (DTOF_RET_SUCCESS != ret) {
                DTOF_CHECK_RET(ret, "gpio config fail");
            }

            ret = device_gpio_write(gpio[i].pin_id, gpio_set[i].pin_default_value);
            if (DTOF_RET_SUCCESS != ret) {
                DTOF_CHECK_RET(ret, "gpio config fail");
            }

            // TBD gpio接口初始化需要增加初始化默认电平设置参数 @Fred
            if (dssal_reset_pin == i) {
                delay_time_us(100);

                ret = device_gpio_write(gpio[i].pin_id, 1);
                if (DTOF_RET_SUCCESS != ret) {
                    DTOF_CHECK_RET(ret, "gpio config fail");
                }

                delay_time_us(750);
            }
            if (dssal_intr_pin == i) {
                // 注册中断回调
                ret = device_gpio_irq_attach(gpio[i].pin_id, 1, _gpio_irq_isr, (void*)&dev->dsd_interrupt.device_id);
                if (DTOF_RET_SUCCESS != ret) {
                    DTOF_CHECK_RET(ret, "gpio irq init fail");
                }
            }
        }
        i++;
    }
    return ret;
}

extern DTOF_RET stm32_uart_init(int uart_id);

static DTOF_RET _ds_device_peripheral_communication_init(dtof_device_t *dev) {
    DTOF_RET ret = DTOF_RET_SUCCESS;
    ds_sal_comm_cfg_t *comm_cfg_p = &dev->dsd_peripheral.common_cfg;

    ret = device_uart_init(UART0_NBR0);
    if(ret != DTOF_RET_SUCCESS)
    {
        DTOF_CHECK_RET(ret, "uart init fail");
    }

    ret = device_init(comm_cfg_p->comm_channel_id);
    if(ret != DTOF_RET_SUCCESS)
    {
        DTOF_CHECK_RET(ret, "communication init fail");
    }

    if(comm_cfg_p->comm_type == COMM_IIC){
        DTOF_BIT_SET(dev->sensor_flags, SENSOR_F_I2C);
        DTOF_BIT_SET(dev->sensor_flags, SENSOR_F_LITTLEENDIAN);
    }else if(comm_cfg_p->comm_type == COMM_SPI){
        DTOF_BIT_CLR(dev->sensor_flags, SENSOR_F_I2C);
        DTOF_BIT_CLR(dev->sensor_flags, SENSOR_F_LITTLEENDIAN);
    }


		return ret;
}
/**
 * @brief 初始化dtof外围接口
 * @param[in]  deviceID dtof device ID
 * @param[in]  peripheralConfig dtof外围接口配置
 * @return DTOF_RET
 */
DTOF_RET ds_device_peripheral_init(ds_sal_config_t *peripheralConfig) {
    DTOF_RET ret = DTOF_RET_SUCCESS;
    dtof_device_t *dev = ds_device_get();
    dtof_memcpy((void*)&dev->dsd_peripheral, (void*)peripheralConfig, sizeof(ds_sal_config_t));
    dev->dsd_interrupt.device_id = 0;

    // 1. 初始化communication接口
    ret = device_driver_ops_init(dev);
    if (DTOF_RET_SUCCESS != ret) {
        DTOF_CHECK_RET(ret, "ops init fail");
    }
    // 2. 初始化gpio
    ret = _ds_device_peripheral_gpio_init(dev);
    if (DTOF_RET_SUCCESS != ret) {
        DTOF_CHECK_RET(ret, "gpio init fail");
    }
    // 3. 初始化communication
    ret = _ds_device_peripheral_communication_init(dev);
    if (DTOF_RET_SUCCESS != ret) {
        DTOF_CHECK_RET(ret, "communication api init fail");
    }
    return ret;
}

/**
 * @brief 获取dtof device控制结构
 * @param[in] deviceID dtof device id
 * @return dtof device指针
 */
dtof_device_t *ds_device_get(void) {
    return &ds_dev;
}

/// @}

static ds_sal_config_t peripheral;

static int board_peripheral()
{
    peripheral.common_cfg.comm_type = COMM_IIC;
    peripheral.common_cfg.comm_channel_id = IIC_NBR0;

    peripheral.pin_cfg[dssal_intr_pin].pin_id = PLATFORM_INTERRUPT_PIN;
    peripheral.pin_cfg[dssal_intr_pin].pin_config = PLATFORM_INTERRUPT_CFG;
    peripheral.pin_set[dssal_intr_pin].pin_default_value = MOS_GPIO_PULL_DOWN;

    peripheral.pin_cfg[dssal_reset_pin].pin_id = PLATFORM_RST_PIN;
    peripheral.pin_cfg[dssal_reset_pin].pin_config = PLATFORM_RST_CFG;
    peripheral.pin_set[dssal_reset_pin].pin_default_value = MOS_GPIO_PULL_DOWN;

    peripheral.pin_cfg[dssal_vcc_en_pin].pin_id = PLATFORM_VCC_PIN;
    peripheral.pin_cfg[dssal_vcc_en_pin].pin_config = PLATFORM_VCC_CFG;
    peripheral.pin_set[dssal_vcc_en_pin].pin_default_value = MOS_GPIO_PULL_UP;

    peripheral.pin_cfg[dssal_vcc1_en_pin].pin_id = PLATFORM_VCC1_PIN;
    peripheral.pin_cfg[dssal_vcc1_en_pin].pin_config = PLATFORM_VCC1_CFG;
    peripheral.pin_set[dssal_vcc1_en_pin].pin_default_value = MOS_GPIO_PULL_UP;

    peripheral.pin_cfg[dssal_1v2_en_pin].pin_id = PLATFORM_VCC1V2_PIN;
    peripheral.pin_cfg[dssal_1v2_en_pin].pin_config = PLATFORM_VCC1V2_CFG;
    peripheral.pin_set[dssal_1v2_en_pin].pin_default_value = MOS_GPIO_PULL_UP;

    peripheral.pin_cfg[dssal_1v8_en_pin].pin_id = PLATFORM_VCC1V8_PIN;
    peripheral.pin_cfg[dssal_1v8_en_pin].pin_config = PLATFORM_VCC1V8_CFG;
    peripheral.pin_set[dssal_1v8_en_pin].pin_default_value = MOS_GPIO_PULL_UP;

    return 0;
}

DTOF_RET dtof_peripheral_device_init(void)
{
    /*
     * 1. 初始化gpio
     *  1.1 interrupt pin: (STM32_PIN_MODE_IT_FALLING | STM32_PIN_PULL_PULLUP | STM32_PIN_SPEED_HIGH)
     *  1.2 reset pin: (STM32_PIN_MODE_OUTPUT_PP | STM32_PIN_PULL_NOPULL | STM32_PIN_SPEED_LOW) 默认拉低
     *  1.3 iic pin: (STM32_PIN_MODE_AF_OD | STM32_PIN_PULL_NOPULL | STM32_PIN_SPEED_VERY_HIGH | STM32_PIN_ALT_4)
     * 2. 初始化sensor
     *  2.1 reset pin init 后 usleep(100), 然后拉高reset pin, usleep(750)
     *  2.2 iic初始化
     *  2.3 注册中断
    */
    DTOF_RET ret;

    board_peripheral();

    DTOF_CHECK_RET(ds_device_peripheral_init(&peripheral), "peripheral init fail");

    return DTOF_RET_SUCCESS;
}

DTOF_RET ds_get_chip_type(dtof_uint16_t chip_id, dtof_chip_type_t* chiptype)
{
    DTOF_RET ret;

    DTOF_CHECK_PTR(chiptype);

    if (chip_id != DTOF_CHIP_TYPE_UNKNOWN){
        *chiptype = chip_id;
        ret = DTOF_RET_SUCCESS;
    } else {
        *chiptype = DTOF_CHIP_TYPE_UNKNOWN;
        DTOF_LOG_ERR("unknown chip id: %d", chip_id);
        ret = DTOF_RET_FAILED;
    }

    return ret;
}
