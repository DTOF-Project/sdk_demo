/*
 * dtof_hal.h
 *
 *  Created on: 2024/10/24
 *      Author: liuzihao
 */

#ifndef _DTOF_HAL_H_
#define _DTOF_HAL_H_

#ifdef __cplusplus
extern "C"
{
#endif

#define LASER_DRIVER_DISABLE   0
#define LASER_DRIVER_ENABLE    1

#define PLL_ENABLE 1
#define PLL_DISABLE 0

#define PLL_2_DIVISION  0
#define PLL_4_DIVISION  1
#define PLL_8_DIVISION  2
#define PLL_16_DIVISION 3

#define SWITCH_PLL_CLK 0
#define SWITCH_EXTERNAL_CLK 7
#define SWITCH_LOWER_POWER_CLK 6
#define SWITCH_OSC_CLK 4

#define HVPP_ENABLE 1
#define HVPP_DISABLE 0


DTOF_RET hal_cg_config(dtof_bool_t status);
DTOF_RET hal_dtof_maxfls_config(dtof_uint16_t maxfls);
DTOF_RET hal_dtof_maxfls_get(dtof_uint16_t *maxfls);
DTOF_RET hal_ref_spad_mask_config(dtof_uint16_t ref_spad_mask);
DTOF_RET hal_next_ac_data_config(dtof_uint16_t *next_ac_data_p, dtof_uint16_t length);
DTOF_RET hal_next_dc_data_config(dtof_uint16_t next_dc_data);
#define INNER_OTP_START_ADDR 0X0800
#define INNER_OTP_SIZE 0X80
DTOF_RET dtof_read_otp(dtof_uint8_t offset, dtof_uint8_t *buf, dtof_uint16_t len);
DTOF_RET dtof_write_otp(dtof_uint8_t offset, dtof_uint16_t len, dtof_uint8_t *out_buf);

DTOF_RET hal_dtof_burst_read_ram(dtof_uint16_t start_addr, dtof_uint16_t len, dtof_uint16_t *buf);
DTOF_RET hal_dtof_burst_write_ram(dtof_uint16_t start_addr, dtof_uint16_t len, dtof_uint16_t *buf);

#define DTOF_FIFO_START_ADDR   0x0400
#define DTOF_SINGLE_FIFO_LEN   0x0019
#define DTOF_MULTIPLE_FIFO_LEN 0x001B
#define DTOF_FIFO_LEN (DTOF_SINGLE_FIFO_LEN + DTOF_MULTIPLE_FIFO_LEN)
#define DTOF_SINGLE_MAIN_HISTGRAM_OFFSET   0
#define DTOF_SINGLE_MAIN_HISTGRAM_LEN      512 // 64 * 8
#define DTOF_SINGLE_REF_HISTGRAM_OFFSET    512
#define DTOF_SINGLE_REF_HISTGRAM_LEN       64
DTOF_RET dtof_histgram_io_read(dtof_uint16_t address, dtof_uint16_t * value_p, dtof_uint16_t len);
DTOF_RET dtof_dsp_fifo_read(dtof_uint16_t addr_offset, dtof_uint16_t * value_p, dtof_uint16_t len);

#define ENABLE_FSM   1
#define DISABLE_FSM  0
DTOF_RET dtof_fsm_change(dtof_uint32_t runmode);
dtof_uint16_t dtof_fsm_state_get(void);
DTOF_RET hal_dtof_prepare_one_frame(void);


#define DTOF_SLEEP  1
#define DTOF_WAKEUP 0
DTOF_RET hal_dtof_set_sleep(dtof_uint16_t status);

#define RESET_TYPE_SOFT  0
#define RESET_TYPE_GLOBAL  1
DTOF_RET hal_dtof_reset(dtof_uint16_t reset_type);

#ifdef __cplusplus
}
#endif

#endif // _DTOF_HAL_H_
