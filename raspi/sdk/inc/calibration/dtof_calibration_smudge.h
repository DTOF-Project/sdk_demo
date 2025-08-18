/**
 * @file dtof_calibration_smudge.h
 * @author jiao.xu
 * @brief about chip smudge calibration
 * @version 1.0
 * @date 2022-08-11
 *
 * @copyright Copyright (c) 2022
 *
 */

#ifndef _DTOF_CALIBRATION_SMUDGE_H_
#define _DTOF_CALIBRATION_SMUDGE_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include "inc/dtof_base_type.h"

// define nosise amb index
// INDEX base 0 and end 31
#define NOISE_AMB_SELECT_INDEX0 1
#define NOISE_AMB_SELECT_INDEX1 2
#define NOISE_AMB_SELECT_INDEX2 28
#define NOISE_AMB_SELECT_INDEX3 29
#define NOISE_AMB_SELECT_INDEX4 30
#define NOISE_AMB_SELECT_INDEX5 31
#define NOISE_AMB_MAX_INDEX 6

// define smudge calibrate moving average simulation parameter
#define DTOF_SMUDGE_MOVING_N 32
#define DTOF_SMUDGE_DIFF_TH (1.4f)
#define DTOF_SMUDGE_PEAK_IDX 3
#define DTOF_SMUDGE_PEAK_LEN 8
#define DTOF_SMUDGE_NOISE_IDX 27
#define DTOF_SMUDGE_NOISE_LEN 5

// Smudge Indicates the minimum distance to calibrate the smudge barrier.
// The deviceID is bin
#define SMUDGE_CALIB_OBJECT_LIMIT_DIS 40
#define SMUDGE_CALIB_FREQ 1 // 多少帧做一次smudge校准

/**
 * @brief  init smudge calibrate module.
 * @param  deviceID: device id.
 * @retval See the details in dtof_errno.h.
 */
DTOF_RET dtof_smudge_calibrate_init(void);
/**
 * @brief  Deinit smudge calibrate module.
 * @param  deviceID: device id.
 * @retval See the details in dtof_errno.h.
 */
DTOF_RET dtof_smudge_calibrate_deinit(void);
/**
 * @brief  set smudge calibrate module enalbe.
 * @param  deviceID: device id.
 * @param  enable: input DTOF_FALSE-disable module; DTOF_TRUE-enable module.
 * @retval See the details in dtof_errno.h.
 */
DTOF_RET dtof_set_smudge_calibrate_module_enable(dtof_bool_t enable);
/**
 * @brief 设置crosstalk参数到smudge模块.
 * @param  deviceID: device id.
 * @param  cross_talk_calib_data: crosstalk 32个bin原始数据
 * @param  size: 等于32
 * @param cross_talk_nextbstr: input cross talk calibrate nextbstr ratio.
 * @retval See the details in dtof_errno.h.
 * @note smudge校准处理前必须调用该接口设置smudge模块crosstalk参数.
 */
DTOF_RET dtof_smudge_set_crosstalk_param(dtof_uint16_t *cross_talk_calib_data,
                                            dtof_uint16_t size,
                                            dtof_uint16_t cross_talk_nextbstr);
/**
 * @brief 清除设置到smudge模块的crosstalk参数.
 * @param  deviceID: device id.
 * @retval See the details in dtof_errno.h.
 * @note crosstalk参数被清除时也需要清除smudge校准模块参数.
 */
DTOF_RET dtof_smudge_clear_crosstalk_param(void);
/**
 * @brief smudge calibrate process.
 * @param  deviceID: device id.
 * @param cross_talk_hisg: input Real-time cross talk histogram.
 * @param size: input cross talk histogram size.
 * @param flashn: input FIFO flashn.
 * @param mp0: input FIFO mp0.
 * @retval See the details in dtof_errno.h.
 */
DTOF_RET dtof_smudge_calibrate(const uint16_t *cross_talk_hisg,
                                    uint16_t size, uint16_t flashn,
                                    uint16_t mp0);

#ifdef __cplusplus
}
#endif

#endif // _DTOF_CALIBRATION_SMUDGE_H_
