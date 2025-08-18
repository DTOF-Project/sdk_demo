/**
 * @file dtof_calibration_cg.h
 * @author fred
 * @brief 芯片串扰校准相关功能
 * @version 1.0
 * @date 2022-08-11
 *
 * @copyright Copyright (c) 2022
 */

#ifndef _DTOF_CALIBRATION_CG_H_
#define _DTOF_CALIBRATION_CG_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include "inc/dtof_base_type.h"
#include "sdk/inc/calibration/dtof_calibration_common.h"

// debug开关
// #define DTOF_CT_DEBUG

// 串扰校准相关参数定义
#define DTOF_CG_CALIB_FRAME_SIZE      100    // 采样帧数
#define DTOF_CG_CALIB_FLASH_CNT       0x7fff // Flash计数最大值

// 数据补偿相关参数定义
#define DTOF_CT_CAL_SLOPE             (-0.0769f)
#define DTOF_CT_CAL_SLOPE_START_BIN   39
#define DTOF_CT_CAL_SLOPE_END_BIN     64
#define DTOF_CT_CAL_SLOPE_BIN_SIZE    (DTOF_CT_CAL_SLOPE_END_BIN - DTOF_CT_CAL_SLOPE_START_BIN + 1)

typedef struct {
    dtof_uint16_t cross_talk_sample_data[DTOF_CT_BIN_SIZE_NEW];
    dtof_uint16_t cross_talk_bin_data[DTOF_CT_TEMP_SIZE]; // smudge use
    dtof_uint16_t next_ac[DTOF_CT_PARAM_SIZE];
    dtof_uint16_t next_dc;
} cross_talk_data_t;

/**
 * @brief 初始化串扰校准模块
 * @return DTOF_RET 初始化结果
 */
DTOF_RET dtof_cross_talk_calibrate_init(void);

/**
 * @brief 反初始化串扰校准模块
 * @return DTOF_RET 反初始化结果
 */
DTOF_RET dtof_cross_talk_calibrate_deinit(void);

/**
 * @brief 设置串扰校准模块使能状态
 * @param enable DTOF_TRUE-使能模块; DTOF_FALSE-禁用模块
 * @return DTOF_RET 设置结果
 */
DTOF_RET dtof_set_cross_talk_calibrate_module_enable(dtof_bool_t enable);

/**
 * @brief 串扰校准数据采样
 * @param frame_size 采样帧数，不超过 DTOF_CG_CALIB_FRAME_SIZE
 * @param[out] cross_talk_calib_data 归一化后的32个bin数据
 * @return DTOF_RET 采样结果
 */
DTOF_RET dtof_cross_talk_sample_data(uint16_t frame_size,
                                    uint16_t *cross_talk_calib_data);

/**
 * @brief 计算串扰校准参数
 * @param cross_talk_histgram 32个bin原始数据
 * @param[out] cross_talk_bin_data bin数据输出
 * @param[out] cross_talk_ac_data 80-8F寄存器值
 * @param[out] cross_talk_dc_data 90寄存器值
 * @return DTOF_RET 计算结果
 */
DTOF_RET dtof_cross_talk_calibrate_calculate(const uint16_t *cross_talk_histgram,
                                            uint16_t* cross_talk_bin_data,
                                            uint16_t *cross_talk_ac_data,
                                            uint16_t *cross_talk_dc_data);

DTOF_RET dtof_do_cross_talk_calibration(int32_t type, cross_talk_data_t *cross_talk_data_p);

#ifdef __cplusplus
}
#endif

#endif // _DTOF_CALIBRATION_CG_H_
