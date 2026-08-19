/**
 * @file dtof_customer.h
 * @brief DTOF客户接口定义
 * @author liuzihao
 * @date 2025/1/6
 */

#ifndef _DTOF_CUSTOMER_H_
#define _DTOF_CUSTOMER_H_

#include <stdint.h>
#include "inc/dtof_base_type.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 批量写入寄存器
 * @param reg_addr 起始寄存器地址
 * @param reg_data_p 数据缓冲区指针
 * @param len 数据长度(字)
 * @return DTOF_RET_SUCCESS 成功
 *         DTOF_RET_INVALID_PARAM 参数无效
 *         DTOF_RET_DEVICE_ERROR 设备错误
 */
int dtof_reg_burst_write(uint8_t reg_addr, uint16_t *reg_data_p, uint16_t len);
int dtof_reg_burst_write_burn(uint8_t reg_addr, const uint16_t *reg_data_p, uint16_t len);

/**
 * @brief 批量读取寄存器
 * @param reg_addr 起始寄存器地址
 * @param reg_data_p 数据缓冲区指针
 * @param len 数据长度(字)
 * @return DTOF_RET_SUCCESS 成功
 *         DTOF_RET_INVALID_PARAM 参数无效
 *         DTOF_RET_DEVICE_ERROR 设备错误
 */
int dtof_reg_burst_read(uint8_t reg_addr, uint16_t *reg_data_p, uint16_t len);

/**
 * @brief 设置中断标志
 * @param flag 中断标志值
 */
void dtof_set_interrupt_flag(dtof_bool_t flag);

/**
 * @brief 获取中断标志
 * @return 当前中断标志值
 */
dtof_bool_t dtof_get_interrupt_flag(void);

/**
 * @brief ms延时
 * @param time 延时时间(ms)
 */
void dtof_sleep_ms(dtof_uint32_t time);

#ifndef FLASH_PAGE_SIZE
#define FLASH_PAGE_SIZE                 0x800U  /* 2 KB */
#endif

#define DTOF_FT_DATA_FLASH_PAGE 224
#define DTOF_FT_DATA_FLASH_PAGE_START_ADDR (0x08000000 + DTOF_FT_DATA_FLASH_PAGE * FLASH_PAGE_SIZE)
#define DTOF_FT_DATA_FLASH_PAGE_NUM 1

void stm32_flash_write_init(uint32_t page, uint32_t page_num);
DTOF_RET dtof_get_ft_data_from_flash(dtof_uint16_t *ft_data, dtof_uint16_t len, dtof_bool_t *is_legal_data);
DTOF_RET dtof_get_ft_data_from_flash_multi_mode(dtof_uint16_t *ft_data, dtof_uint16_t len, dtof_run_mode_e run_mode, dtof_bool_t *is_legal_data);
DTOF_RET dtof_set_ft_data_to_flash(dtof_uint16_t *ft_data, dtof_uint16_t len);
DTOF_RET dtof_set_ft_data_to_flash_multi_mode(dtof_uint16_t *ft_data, dtof_uint16_t len, dtof_run_mode_e run_mode);

void stm32_flash_write_u64(uint32_t offset, uint64_t *context, uint16_t num_words);
void stm32_flash_read_u64(uint32_t offset, uint64_t *context, uint16_t num_words);
Sensor_Status Sensor_IIC_Read_One_Byte(uint8_t addr,uint8_t *value);
Sensor_Status Sensor_IIC_Write_One_Byte(uint8_t addr,uint8_t value);
Sensor_Status Sensor_IIC_Write_X_Bytes(uint8_t addr,uint8_t *pValue,uint16_t tlen);
Sensor_Status Sensor_IIC_Read_X_Bytes(uint8_t addr,uint8_t *value,uint16_t tlen);
void Sensor_Delay_Ms(uint16_t nMs);

#ifdef __cplusplus
}
#endif

#endif
