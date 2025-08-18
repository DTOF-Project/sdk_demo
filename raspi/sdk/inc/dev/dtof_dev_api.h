/*
 * dtof_io_interaction.h
 *
 *  Created on: 2024/8/5
 *      Author: liuzihao
 */

#ifndef _DTOF_DEV_API_H_
#define _DTOF_DEV_API_H_

#ifdef __cplusplus
extern "C" {
#endif

DTOF_RET dtof_bypass_inner_mcu_external(void);
DTOF_RET dtof_wakeup_inner_mcu_external(void);

DTOF_RET dtof_get_multiple_result(dtof_uint16_t *mul_result_p);
dtof_uint32_t dtof_get_mul_read_times(void);
void dtof_set_mul_read_times(dtof_uint32_t mul_read_times);
dtof_bool_t dtof_get_mul_read_flag(void);
void dtof_set_mul_read_flag(dtof_bool_t mul_read_flag);
void dtof_init_mul_res_temp(void);

#define DTOF_MODE_SMOKE               0
#define DTOF_MODE_0_2_HZ_ATN_DISTANCE 1
#define DTOF_MODE_1_HZ_ATN_DISTANCE   2
#define DTOF_MODE_INFRARED            3
#define DTOF_MODE_MUL_CP_FLAG         4
#define DTOF_MODE_MUL_POS_FLAG        5
#define DTOF_MODE_MUL_POS_ALGO_FLAG   6
#define DTOF_MODE_30_HZ_DISTANCE      7
#define DTOF_MODE_20_HZ_DISTANCE      8
#define DTOF_MODE_10_HZ_DISTANCE      9
DTOF_RET dtof_switch_mode(dtof_uint16_t mode, dtof_uint16_t* reg_config_p, dtof_uint16_t len);

#define DTOF_SWITCH_MODE_CMD   0x18
#define DTOF_SWITCH_MODE_NORMAL   0
#define DTOF_SWITCH_SMOKE_MODE 0
#define DTOF_SWITCH_INFRARED_MODE 3
#define DTOF_INFRARED_MODE 0X5
#define DTOF_INFRARED_CMD_VALID 0X6
#define DTOF_INFRARED_CMD_REPEAT 0X7
#define DTOF_INFRARED_CMD_QUIT 0X8
DTOF_RET dtof_switch_infrared_mode(void);
DTOF_RET dtof_start_send_infrared(void);
DTOF_RET dtof_send_infrared_info(dtof_uint8_t value, dtof_uint32_t mode);
DTOF_RET dtof_quit_infrared_mode(void);
DTOF_RET dtof_write_otp_byte(dtof_uint16_t value, dtof_uint16_t time);

#define DTOF_SWITCH_MUTILPLE_MODE_CMD 0x09
#define MODE_FLAG_MUL_CP_FLAG     4
#define MODE_FLAG_MUL_POS_FLAG    5
#define MODE_FLAG_MUL_POS_ALGO_FLAG 6
DTOF_RET dtof_switch_multiple_mode(dtof_uint32_t mode);
DTOF_RET dtof_switch_multiple_mode_another(dtof_uint32_t mode);

#define DTOF_CMD_MANUAL_CONTROL 0x10
#define DTOF_START_FRAME 0x0
#define DTOF_STOP_FRAME  0x1
DTOF_RET dtof_manual_control(dtof_uint32_t mode);

#define DTOF_CMD_CHANGE_REPORT_RESULT_MODE 0x11
    #define DTOF_REPORT_TRIGGER     0
    #define DTOF_REPORT_DROP_DOWN   1
    #define DTOF_REPORT_PULL_UP     2
DTOF_RET dtof_change_report_result_mode(dtof_uint32_t report_mode);
#define DTOF_CMD_CHANGE_FSM_STATUS       0X13
    #define DTOF_FSM_FREE_RUN       0
    #define DTOF_FSM_RUN_ONCE       1
    #define DTOF_FSM_RUN_DELAY      2
    #define DTOF_FSM_HOLD_RUN       3
DTOF_RET dtof_change_fsm_status(dtof_uint32_t fsm_status);

#define DTOF_CMD_SET_SMOKE_IGNORE_FRAME 0x14
DTOF_RET dtof_set_smoke_ignore_frame(dtof_uint16_t ignore_frame);

#define DTOF_CMD_RESET_AND_CLOSE_WDT 0x15
DTOF_RET dtof_reset_and_close_wdt(void);

#define DTOF_CMD_STOP_ACCUMULATE_MODE   0X16
DTOF_RET dtof_stop_accumulate_mode(void);

#define DTOF_CMD_DEBUG              0x3F
    #define DTOF_CMD_DEBUG_KEEP_CJTAG    1
    #define DTOF_CMD_DEBUG_NO_KEEP_CJTAG 2
    #define DTOF_CMD_DEBUG_ENABLE_CJATG  3
    #define DTOF_CMD_DEBUG_DISABLE_CJATG 4
DTOF_RET dtof_debug(dtof_uint32_t mode);
dtof_real32_t dtof_iic_calculate(dtof_real32_t iic_speed);
DTOF_RET dtof_iic_calculate_ret(dtof_real32_t iic_speed);

DTOF_RET dtof_ram_output_dsp_fifo(void);
DTOF_RET dtof_ram_output_dsp_fifo_low_power(void);
DTOF_RET dtof_ram_output_dsp_fifo_debug(void);

#define DTOF_DEV_MODE
#ifdef DTOF_DEV_MODE
#undef DTOF_DEV_MODE
DTOF_RET dtof_read_reg_running(dtof_uint16_t reg_addr, dtof_uint16_t *reg_data);
DTOF_RET dtof_write_reg_running(dtof_uint16_t reg_addr, dtof_uint16_t reg_data);
#endif

typedef struct
{
    dtof_uint64_t temp_result;
    dtof_uint32_t result;
} dtof_smoke_result_t ;

#ifdef __cplusplus
}
#endif

#endif // _DTOF_DEV_API_H_
