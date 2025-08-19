/**
 * @file dtof_calibration_timestamp.h
 * @author jiao.xu
 * @brief about chip timestamp calibration
 * @version 1.0
 * @date 2022-08-11
 *
 * @copyright Copyright (c) 2022
 *
 */

#ifndef _DTOF_CALIBRATION_TIMESTAMP_H_
#define _DTOF_CALIBRATION_TIMESTAMP_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include "inc/dtof_base_type.h"

/**
 * @brief  init timestamp calibrate module.
 * @param  pHandle: input timestamp calibrate module Handle.
 * @retval See the details in dtof_errno.h.
 * @note   timestamp calibrate control parameter init
 */
DTOF_RET dtof_timestamp_calibrate_init(void);
/**
 * @brief  Deinit timestamp calibrate module.
 * @param  pHandle: input timestamp calibrate module Handle.
 * @retval See the details in dtof_errno.h.
 */
DTOF_RET dtof_timestamp_calibrate_deinit(void);
/**
 * @brief  set timestamp calibrate module enalbe.
 * @param  pHandle: input timestamp calibrate module Handle.
 * @param  enable: input DTOF_FALSE-disable module; DTOF_TRUE-enable module.
 * @retval See the details in dtof_errno.h.
 */
DTOF_RET dtof_set_timestamp_calibrate_module_enable(dtof_bool_t enable);
/**
 * @brief  start timestamp calibrate count.
 * @param  pHandle: input timestamp calibrate module Handle.
 * @retval See the details in dtof_errno.h.
 * @note   This function should be called when start capturing a frame
 */
DTOF_RET dtof_timestamp_calibrate_start_count(void);
/**
 * @brief  stop timestamp calibrate count.
 * @param  pHandle: input timestamp calibrate module Handle.
 * @retval See the details in dtof_errno.h.
 * @note   This function should be called when capturing a frame
 * finish(interrupt comes).
 */
DTOF_RET dtof_timestamp_calibrate_stop_count(void);
/**
 * @brief  process timestamp calibrate for k/b.
 * @param  pHandle: input timestamp calibrate module Handle.
 * @param  flashn: input fifo flashn.
 * @retval See the details in dtof_errno.h.
 */
DTOF_RET dtof_timestamp_calibrate_process(uint16_t flashn);
/**
 * @brief  block for capturing a frame by timestamp calibrate.
 * @param  pHandle: input timestamp calibrate module Handle.
 * @param  time_out_ms: input time out for block.
 * @retval See the details in dtof_errno.h.
 */
DTOF_RET dtof_timestamp_calibrate_block(uint32_t time_out_ms);
/**
 * @brief  update timestamp calibrate vcsel trigle cycle.
 * @param  pHandle: input timestamp calibrate module Handle.
 * @retval See the details in dtof_errno.h.
 * @note   This function should be called when register(PLL_CTRL:0xA2) change.
 */
DTOF_RET dtof_timestamp_calibrate_update_vcsel_trig_cycle(void);
/**
 * @brief  timestamp calibrate distance(dtof_real32_t).
 * @param  pHandle: input timestamp calibrate module Handle.
 * @param  in_dis: input distance which need to be calibrated.
 * @param  out_dis: output timestamp calibrate distance.
 * @retval See the details in dtof_errno.h.
 */
DTOF_RET dtof_timestamp_calibrate_calculate_float(dtof_real32_t in_dis, dtof_real32_t *out_dis);
/**
 * @brief  timestamp calibrate distance(int16_t).
 * @param  pHandle: input timestamp calibrate module Handle.
 * @param  in_dis: input distance which need to be calibrated.
 * @param  out_dis: output timestamp calibrate distance.
 * @retval See the details in dtof_errno.h.
 */
DTOF_RET dtof_timestamp_calibrate_calculate_int16(int16_t in_dis, int16_t *out_dis);
/**
 * @brief  get timestamp calibrate parameter:k/b.
 * @param  pHandle: input timestamp calibrate module Handle.
 * @param  k: output timestamp calibrate k value.
 * @param  b: output timestamp calibrate b value.
 * @retval See the details in dtof_errno.h.
 */
DTOF_RET dtof_get_timestamp_calibrate_param(dtof_real32_t *k, dtof_real32_t *b);

#ifdef __cplusplus
}
#endif

#endif // _DTOF_CALIBRATION_TIMESTAMP_H_
