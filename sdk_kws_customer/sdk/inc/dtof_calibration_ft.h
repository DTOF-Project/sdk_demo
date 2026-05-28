#ifndef _DTOF_CALIBRATION_FT_H_
#define _DTOF_CALIBRATION_FT_H_

#ifdef __cplusplus
extern "C" {
#endif

#define DTOF_FT_CALIBRATE_BINOFFSET 0
#define DTOF_FT_CALIBRATE_REFSPAD   1
#define DTOF_FT_CALIBRATE_CG        2
#define DTOF_FT_CALIBRATE_B         3

#include "inc/dtof_base_type.h"
#include "inc/dtof_api.h"
#include "inc/lib/dtof_ft.h"

#define DTOF_FT_K_MULTIPLE  64
#define DTOF_FT_B_MULTIPLE  8

typedef struct {
    binoffset_cal_t     binoffset_cal_data;
    ref_spad_cal_t      ref_spad_cal;
    cross_talk_data_t   cross_talk_data;
    kb_data_t           kb_data;
} dtof_calibrate_data_ft_t;

DTOF_RET dtof_do_ft_calibration(dtof_run_mode_e run_mode, dtof_uint16_t ft_cali_type, dtof_uint16_t ft_actual_param);
DTOF_RET dtof_do_ft_calibration_all_mode(dtof_uint16_t ft_cali_type, dtof_uint16_t ft_actual_param);

#ifdef __cplusplus
}
#endif

#endif
