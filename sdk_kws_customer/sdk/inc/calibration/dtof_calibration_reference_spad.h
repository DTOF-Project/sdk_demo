/**
 * @file dtof_calibration_reference_spad.h
 * @author jiao.xu
 * @brief about chip reference spad calibration
 * @version 1.0
 * @date 2022-08-11
 *
 * @copyright Copyright (c) 2022
 *
 */

#ifndef _DTOF_CALIBRATION_REFERENCE_SPAD_H_
#define _DTOF_CALIBRATION_REFERENCE_SPAD_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include "inc/dtof_base_type.h"

#define DTOF_REF_SPAD_MAX_NUM          8     // refspad 最大个数
#define DTOF_REF_SPAD_MV1_SUM_MAX      8192  // MV1累计最大值
#define DTOF_REF_SPAD_MV1_RAM_ADDR     0x40d // MV1 RAM地址

typedef struct {
    dtof_uint16_t spad_index;    // 改进命名，移除拼写错误
    dtof_uint16_t spad_value;
    dtof_bool_t   is_valid;      // 添加有效性标志
} dtof_spad_info_t;              // 统一命名规范

/**
 * @brief  init ref spad calibrate module.
 * @param  deviceID: device id.
 * @retval See the details in dtof_errno.h.
 */
DTOF_RET dtof_ref_spad_calibrate_init(void);
/**
 * @brief  Deinit ref spad calibrate.
 * @param  deviceID: device id.
 * @retval See the details in dtof_errno.h.
 */
DTOF_RET dtof_ref_spad_calibrate_deinit(void);
/**
 * @brief  set ref spad calibrate module enalbe.
 * @param  deviceID: device id.
 * @param  enable: input DTOF_FALSE-disable module; DTOF_TRUE-enable module.
 * @retval See the details in dtof_errno.h.
 */
DTOF_RET dtof_set_ref_spad_calibrate_module_enable(dtof_bool_t enable);



/**
 * @brief  ref spad calibration.
 * @param  otp_ref_spad_mask: OTP store ref spad mask.
 * @param  ref_spad_cal: output ref spad calibrate result.
 * @retval See the details in dtof_errno.h.
 *
 *  1. close all ref spad.
 *  2. open a spad at one time.
 *  3. sort.
 *  4. select spad.
 *  5. update register after calibrating.
 */
DTOF_RET dtof_ref_spad_calibrate(dtof_uint16_t otp_ref_spad_mask, dtof_uint16_t *ref_spad_cal, dtof_spad_info_t *spad_info);

#ifdef __cplusplus
}
#endif

#endif // _DTOF_CALIBRATION_REFERENCE_SPAD_H_
