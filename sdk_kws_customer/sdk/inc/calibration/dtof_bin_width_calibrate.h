#ifndef _DTOF_BIN_WIDTH_CALIBRATE_H_
#define _DTOF_BIN_WIDTH_CALIBRATE_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "inc/dtof_base_type.h"
#include "inc/calibration/dtof_calibration.h"

// FIFO地址相关定义
#define DTOF_FIFO_ADDR_SINGLE_SIZE    0x0019

// 峰值位置索引定义
#define DTOF_NEAR_PEAK_POS_INDEX      0
#define DTOF_FAR_PEAK_POS_INDEX       1

/**
 * @brief 执行距离校准
 * @param far_distance 远距离值
 * @param kb_data_p KB数据指针
 * @return DTOF_RET 校准结果
 */
DTOF_RET dtof_do_distance_calibration(dtof_uint16_t far_distance, kb_data_t *kb_data_p);

/**
 * @brief 执行旧版距离校准
 * @param distance 距离值
 * @param cal_type 校准类型
 * @param calibrate_data_p 校准数据指针
 * @return DTOF_RET 校准结果
 */
DTOF_RET dtof_do_distance_calibration_old(dtof_uint16_t distance,
                                         dtof_int32_t cal_type,
                                         dtof_calibrate_data_t *calibrate_data_p);

/**
 * @brief 执行B版本距离校准
 * @param distance 距离值
 * @param cal_type 校准类型
 * @param calibrate_data_p 校准数据指针
 * @return DTOF_RET 校准结果
 */
DTOF_RET dtof_do_distance_calibration_b(dtof_uint16_t distance,
                                       dtof_int32_t cal_type,
                                       dtof_calibrate_data_t *calibrate_data_p);

/**
 * @brief 性能校准, 包括snr fwhm, 用来判断芯片性能
 * @param performance_cal_p 校准数据指针
 * @return DTOF_RET 校准结果
 */
DTOF_RET dtof_performance_verify(performance_cal_t* performance_cal_p);

#ifdef __cplusplus
}
#endif

#endif // _DTOF_BIN_WIDTH_CALIBRATE_H_
