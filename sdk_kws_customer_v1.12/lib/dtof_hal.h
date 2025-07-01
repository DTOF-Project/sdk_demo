#ifndef _DTOF_HAL_H_
#define _DTOF_HAL_H_

#ifdef __cplusplus
extern "C"
{
#endif

dtof_uint16_t dtof_fsm_state_get(dtof_uint8_t device_id);
DTOF_RET dtof_histgram_io_read_lib(dtof_uint8_t device_id, dtof_uint16_t address, dtof_uint16_t * value_p, dtof_uint16_t len);
DTOF_RET dtof_fsm_change(dtof_uint8_t device_id, dtof_uint32_t runmode);
DTOF_RET dtof_calibration_get_frame_data(dtof_uint8_t device_id, uint16_t offset, uint16_t *out_buf, uint16_t len);
DTOF_RET dtof_read_reg_running_lib(dtof_uint8_t device_id, dtof_uint16_t reg_addr, dtof_uint16_t *reg_data);
DTOF_RET dtof_write_reg_running_lib(dtof_uint8_t device_id, dtof_uint16_t reg_addr, dtof_uint16_t reg_data);
DTOF_RET hal_dtof_maxfls_get(dtof_uint8_t device_id, dtof_uint16_t *maxfls);
DTOF_RET hal_dtof_maxfls_config(dtof_uint8_t device_id, dtof_uint16_t maxfls);

#ifdef __cplusplus
}
#endif

#endif // _DTOF_HAL_H_
