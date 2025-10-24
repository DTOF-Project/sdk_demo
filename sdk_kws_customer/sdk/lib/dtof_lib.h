#include "inc/dtof_base_type.h"

int dtof_clac_confidence(int16_t first_target, uint16_t first_intensity, float ambient);
DTOF_RET dtof_do_xtalk_calibration(dtof_uint8_t device_id, dtof_uint16_t* ft_data, dtof_int16_t* pos_cal_result, dtof_uint16_t* max_ratio_cal_result);
