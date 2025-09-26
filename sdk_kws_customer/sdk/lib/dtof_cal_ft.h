#ifndef _DTOF_CAL_FT_H_
#define _DTOF_CAL_FT_H_

#ifdef __cplusplus
extern "C"
{
#endif

DTOF_RET dtof_calibration_refbinoffset_calculate(dtof_uint8_t device_id, dtof_uint16_t* binoffset);
DTOF_RET dtof_do_cross_talk_calibration(uint8_t device_id, uint16_t *ct_reg_data);

#ifdef __cplusplus
}
#endif

#endif // _DTOF_CAL_FT_H_
