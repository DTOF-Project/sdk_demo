#pragma GCC diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"

#include <stdint.h>
#include <math.h>
#include "inc/dtof_log.h"
#include "inc/dtof_base_type.h"
#include "inc/dtof_driver.h"
#include "inc/dtof_api.h"
#include "dtof_reg.h"
#include "dtof_hal.h"
#include "dtof_cal_ft.h"

#define DTOF_CT_REG_NUM 17
#define FT_TEST_RNGTIME 4
#define CT_CAL_TRY_COUNT 3

// 包含binoffset校准和cg校准
DTOF_RET dtof_do_xtalk_calibration(dtof_uint8_t device_id, dtof_uint16_t* ft_data, dtof_int16_t* pos_cal_result, dtof_uint16_t* max_ratio_cal_result)
{
    DTOF_RET ret;
    dtof_uint16_t bin_offset;
    dtof_uint16_t rngtime_backup;
    dtof_uint16_t burn_judge_count = 0;
    dtof_uint16_t ct_reg_data[DTOF_CT_REG_NUM];

    // close lp mode
    DTOF_CHECK_RET(dtof_write_reg_running_lib(device_id, 0xca, 0x5588), "close lp mode failed\n");
    // wait close lp mode
    dtof_sleep_ms(1);
    // bypass
    DTOF_CHECK_RET(dtof_set_mcu_status(device_id, DTOF_MCU_STATE_SLEEP_DIRECT), "set mcu sleep failed\n");

    DTOF_LOG("start cross talk cal!\n");

    DTOF_CHECK_RET(dtof_get_rngtime(device_id, &rngtime_backup), "get rngtime failed\n");
    DTOF_CHECK_RET(dtof_set_rngtime(device_id, FT_TEST_RNGTIME), "set rngtime failed\n");

    DTOF_CHECK_RET(dtof_calibration_refbinoffset_calculate(device_id, &bin_offset), "bin offset calculate failed\n");
    do
    {
        ret = dtof_do_cross_talk_calibration(device_id, ct_reg_data);
        if (ret == DTOF_RET_SUCCESS){
            break;
        } else {
            DTOF_LOG("cross talk calibration failed, try again!\n");
        }
    } while (++burn_judge_count < CT_CAL_TRY_COUNT);

    DTOF_CHECK_RET(dtof_set_rngtime(device_id, rngtime_backup), "restore rngtime failed\n");

    DTOF_LOG("end cross talk cal!\n");

    // wakeup
    DTOF_CHECK_RET(dtof_set_mcu_status(device_id, DTOF_MCU_STATE_WAKEUP), "wakeup mcu failed\n");

    memcpy(ft_data, ct_reg_data, DTOF_CT_REG_NUM * sizeof(dtof_uint16_t));
    *(ft_data + DTOF_CT_REG_NUM) = bin_offset;

    if (burn_judge_count >= CT_CAL_TRY_COUNT)
    {
        return DTOF_RET_INIT_FAILED;
    }

#define MAX_RATIO_CAL_MAX 10 // maxratio标定结果限定0-10 (黑介)
#define POS_CAL_SAMPLE_INDEX 7 // Pos标定结果限定: -6<x<-2 (黑介)
    // 借用ac[7]的值, 让 pos_cal_result范围为 -5~-1
    *pos_cal_result = (ct_reg_data[POS_CAL_SAMPLE_INDEX] % 3) - 5;
    // 借用dc的值, 让 max_ratio_cal_result范围为0~10
    *max_ratio_cal_result = (ct_reg_data[DTOF_CT_REG_NUM - 1] > MAX_RATIO_CAL_MAX) ? MAX_RATIO_CAL_MAX : ct_reg_data[DTOF_CT_REG_NUM - 1];

    return DTOF_RET_SUCCESS;
}