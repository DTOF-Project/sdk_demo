#ifndef _DTOF_CALIBRATION_FT_H_
#define _DTOF_CALIBRATION_FT_H_

#ifdef __cplusplus
extern "C" {
#endif

// #define DTOF_FT_CALIBRATE_BINOFFSET
// #define DTOF_FT_CALIBRATE_REFSPAD
#define DTOF_FT_CALIBRATE_CG
// #define DTOF_FT_CALIBRATE_B

#include "inc/dtof_base_type.h"
#include "inc/dtof_api.h"
#include "src/lib/dtof_ft.h"

#define DTOF_FT_K_MULTIPLE  64

typedef struct {
#ifdef DTOF_FT_CALIBRATE_BINOFFSET
    binoffset_cal_t     binoffset_cal_data;
#endif
#ifdef DTOF_FT_CALIBRATE_REFSPAD
    ref_spad_cal_t      ref_spad_cal;
#endif
#ifdef DTOF_FT_CALIBRATE_CG
    cross_talk_data_t   cross_talk_data;
#endif
#ifdef DTOF_FT_CALIBRATE_B
    kb_data_t           kb_data;
#endif
} dtof_calibrate_data_ft_t;

DTOF_RET dtof_do_ft_calibration(dtof_uint16_t otp_ref_spad_mask, dtof_uint16_t distance, dtof_uint16_t is_to_sky);

#ifdef __cplusplus
}
#endif

#endif