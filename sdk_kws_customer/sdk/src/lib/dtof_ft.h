/**
 * @file dtof_ft.h
 * @brief lib 头文件
 * @author liuzihao
 * @date 2025/8/18
 */

#ifndef _DTOF_FT_H_
#define _DTOF_FT_H_

#include <stdint.h>
#include "inc/dtof_base_type.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DTOF_REF_SPAD_MAX_NUM    8     // refspad max num
#define DTOF_AC_NUM              16    // ac num


typedef struct {
    dtof_uint16_t      otp_ref_spad_mask;
    dtof_uint16_t      ref_spad;
}ref_spad_cal_t;

typedef struct {
    dtof_uint16_t spad_index;
    dtof_uint16_t spad_value;
    dtof_bool_t   is_valid;
} dtof_spad_info_t;

typedef struct {
    dtof_uint16_t spad_mask;
    dtof_spad_info_t spad_info[DTOF_REF_SPAD_MAX_NUM];
} refspad_cal_t;

typedef struct {
    dtof_uint16_t binoffset;
} binoffset_cal_t;

typedef struct
{
    dtof_uint16_t next_ac[DTOF_AC_NUM];
    dtof_uint16_t next_dc;
} cross_talk_data_t;

typedef struct {
    dtof_uint16_t far_distance;
    dtof_real32_t k;
    dtof_real32_t b;
} kb_data_t;

DTOF_RET dtof_calibration_refbinoffset_calculate(binoffset_cal_t *binoffset_cal_data_p);
DTOF_RET dtof_ref_spad_calibrate(dtof_uint16_t otp_ref_spad_mask, dtof_uint16_t *ref_spad_cal, dtof_spad_info_t *spad_info);
DTOF_RET dtof_do_cross_talk_calibration(cross_talk_data_t *cross_talk_data_p, dtof_uint16_t is_to_object);
DTOF_RET dtof_do_distance_calibration_b(dtof_uint16_t distance, kb_data_t *calibrate_data_p);
DTOF_RET dtof_do_distance_calibration_b_use_sdk(dtof_uint16_t distance, kb_data_t *calibrate_data_p);

#ifdef __cplusplus
}
#endif

#endif // _DTOF_FT_H_
