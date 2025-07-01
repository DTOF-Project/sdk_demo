/*
 * ds_sal.h
 *
 *  Created on: 2024/9/11
 *      Author: liuzihao
 */
#ifndef SRC_INC_DS_SAL_H_
#define SRC_INC_DS_SAL_H_

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum
{
    dssal_spii2c_pin = 0,     // High : spi  Low:
    dssal_intr_pin,           // interupt
    dssal_algpower_pin,       // analog power supply 3.3v low: disable
    dssal_algpowerdriver_pin, // driver  analog power supply
    dssal_algpowerpump_pin,   // pump supply
    dssal_reset_pin,          // reset pin
    dssal_vcc_en_pin,         // power pin
    dssal_vcc1_en_pin,        // power pin
    dssal_vcc3_en_pin,        // power pin
    dssal_1v2_en_pin,         // power pin
    dssal_1v8_en_pin,         // power pin
    dssal_4v_en_pin,          // power pin
    dssal_epc_en_pin,         // power pin
    dssal_pin_max,
} ds_sal_pin_enum_t;

typedef enum
{
    COMM_IIC = 0,
    COMM_SPI = 1
} ds_sal_communication_enum_t;

typedef struct ds_sal_pinconfig_s
{
    uint16_t pin_id;
    uint32_t pin_config;
} ds_sal_pin_cfg_t;

typedef struct ds_sal_pin_set_s
{
    uint32_t pin_default_value;
} ds_sal_pin_set_t;

typedef struct ds_sal_common_cfg_s
{
    dtof_uint32_t comm_type;        // iic-0, spi-1
    dtof_uint32_t comm_channel_id;
} ds_sal_comm_cfg_t;

typedef struct ds_sal_config_s
{
    ds_sal_comm_cfg_t common_cfg;
    ds_sal_pin_cfg_t pin_cfg[dssal_pin_max];
    ds_sal_pin_set_t pin_set[dssal_pin_max];
} ds_sal_config_t;

#ifdef __cplusplus
}
#endif

#endif //   SRC_INC_DS_SAL_H_
