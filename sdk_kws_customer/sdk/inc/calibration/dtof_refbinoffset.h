#ifndef _DTOF_REFBINOFFSET_H_
#define _DTOF_REFBINOFFSET_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "inc/calibration/dtof_calibration.h"

DTOF_RET dtof_calibration_refbinoffset_calculate(binoffset_cal_t* binoffset_cal_data_p);

#ifdef __cplusplus
}
#endif
#endif