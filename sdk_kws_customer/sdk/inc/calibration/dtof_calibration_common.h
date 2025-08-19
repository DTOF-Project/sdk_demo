#ifndef _DTOF_CALIBRATION_COMMON_H_
#define _DTOF_CALIBRATION_COMMON_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include "inc/dtof_base_type.h"

// 串扰校准相关参数定义
#define DTOF_CT_BIN_SIZE_NEW        65    // 新版本bin大小
#define DTOF_CT_SAMPLE_SIZE         3     // 采样大小
#define DTOF_CT_TEMP_SIZE          ((DTOF_CT_BIN_SIZE_NEW - 1) / 2)
#define DTOF_CT_PARAM_SIZE         (DTOF_CT_TEMP_SIZE / 2)
#define DTOF_CT_BIN_SIZE           32    // 标准bin大小
#define DTOF_CT_FRONT_BIN_SIZE     8     // 前端bin大小

// 数值处理相关参数
#define DTOF_VAL_BOOST_MULTIPLE    10    // 数值提升倍数，用于减少精度损失
#define DTOF_VAL_SAMPLE_MULTIPLE   12    // 采样数据*1.2倍再做CT
#define DTOF_AGC_MAX_FLASH_NUM     65536 // AGC最大闪光次数

#ifndef DTOF_EPSINON
#define DTOF_EPSINON (0.000001f)
#endif

/**
 * @brief 计算向上取整值
 * @param value 输入值
 * @param multiple 倍数
 * @return 向上取整后的结果
 */
uint32_t value_ceil(uint32_t value, uint32_t multiple);

/**
 * @brief 计算四舍五入值
 * @param value 输入值
 * @param multiple 倍数
 * @return 四舍五入后的结果
 */
uint32_t value_round(uint32_t value, uint32_t multiple);

/**
 * @brief 查找uint16数组最大值索引
 * @param buffer 输入数组
 * @param size 数组大小
 * @param[out] maxIndex 最大值索引
 * @return DTOF_RET 操作结果
 */
DTOF_RET dtof_find_max_uint16(const uint16_t *buffer, uint16_t size, uint16_t *maxIndex);

/**
 * @brief 计算int16数组平均值
 * @param data 输入数组
 * @param size 数组大小
 * @param[out] mean 平均值
 * @return DTOF_RET 操作结果
 */
DTOF_RET dtof_calc_mean_int16(const int16_t *data, int16_t size, int16_t *mean);

/**
 * @brief 查找uint32数组最大值
 * @param arr 输入数组
 * @param size 数组大小
 * @return 最大值
 */
uint32_t dtof_find_max_uint32(uint32_t *arr, int32_t size);

/**
 * @brief 计算数组半高宽
 * @param data 输入数组
 * @param length 数组大小
 * @param peak_index 峰值索引
 * @return 半高宽
 */
int dtof_calc_fwhm_uint16(const uint16_t *data, uint32_t length, uint32_t peak_index);

/**
 * @brief 获取校准帧数据
 * @param offset 数据偏移
 * @param[out] out_buf 输出缓冲区
 * @param len 数据长度
 * @return DTOF_RET 操作结果
 */
DTOF_RET dtof_calibration_get_frame_data(uint16_t offset, uint16_t *out_buf, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif // _DTOF_CALIBRATION_COMMON_H_