/**
 * @file dtof_customer.h
 * @brief DTOF客户接口定义
 * @author liuzihao
 * @date 2025/1/6
 */

#ifndef _DTOF_CUSTOMER_H_
#define _DTOF_CUSTOMER_H_
#include "inc/dtof_driver.h"
#include <stdint.h>
#include "inc/dtof_base_type.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {
    DTOF_CHIP_TYPE_A05 = 0x0001,
    DTOF_CHIP_TYPE_L3 = 0x4120,
    DTOF_CHIP_TYPE_UNKNOWN = 0xffff,
} dtof_chip_type_t;
/**
 * @brief 批量写入寄存器
 * @param reg_addr 起始寄存器地址
 * @param reg_data_p 数据缓冲区指针
 * @param len 数据长度(字)
 * @return DTOF_RET_SUCCESS 成功
 *         DTOF_RET_INVALID_PARAM 参数无效
 *         DTOF_RET_DEVICE_ERROR 设备错误
 */
// DTOF_RET dtof_reg_burst_write(uint8_t device_id, uint8_t reg_addr, uint16_t *reg_data_p, uint16_t len);
// DTOF_RET dtof_reg_burst_write_burn(uint8_t device_id, uint8_t reg_addr, const uint16_t *reg_data_p, uint16_t len);

/**
 * @brief 批量读取寄存器
 * @param reg_addr 起始寄存器地址
 * @param reg_data_p 数据缓冲区指针
 * @param len 数据长度(字)
 * @return DTOF_RET_SUCCESS 成功
 *         DTOF_RET_INVALID_PARAM 参数无效
 *         DTOF_RET_DEVICE_ERROR 设备错误
 */
// DTOF_RET dtof_reg_burst_read(uint8_t device_id, uint8_t reg_addr, uint16_t *reg_data_p, uint16_t len);

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

DTOF_RET dtof_get_distance_offset_from_flash(dtof_uint8_t device_id, dtof_int32_t *distance_offset);
DTOF_RET dtof_set_distance_offset_to_flash(dtof_uint8_t device_id, dtof_int32_t distance_offset);
DTOF_RET dtof_get_xtalk_data_from_flash(dtof_uint8_t device_id, dtof_uint16_t *xtalk_data);
DTOF_RET dtof_set_xtalk_data_from_flash(dtof_uint8_t device_id, dtof_uint16_t *xtalk_data, dtof_int16_t pos_cal_result, dtof_uint16_t maxratio_cal_result);
DTOF_RET dtof_read_otp(dtof_uint8_t offset, dtof_uint8_t *buf, dtof_uint16_t len);
/**
 * @brief 外设初始化
 * @return DTOF_RET_SUCCESS 成功
 *         DTOF_RET_FAILED 失败
 */
// DTOF_RET dtof_peripheral_device_init(void);

// #define FLASH_PAGE_SIZE 2048
#define DTOF_CG_DATA_FLASH_PAGE 224
#define DTOF_CG_DATA_FLASH_PAGE_START_ADDR (0x08000000 + DTOF_CG_DATA_FLASH_PAGE * FLASH_PAGE_SIZE)
#define DTOF_CG_DATA_FLASH_PAGE_NUM 1
#define DTOF_B_DATA_FLASH_PAGE (DTOF_CG_DATA_FLASH_PAGE + DTOF_CG_DATA_FLASH_PAGE_NUM)
#define DTOF_B_DATA_FLASH_PAGE_START_ADDR (0x08000000 + DTOF_B_DATA_FLASH_PAGE * FLASH_PAGE_SIZE)
#define DTOF_B_DATA_FLASH_PAGE_NUM 1
// void stm32_flash_write_init(uint32_t page, uint32_t page_num);

#ifdef __cplusplus
}
#endif

#endif
