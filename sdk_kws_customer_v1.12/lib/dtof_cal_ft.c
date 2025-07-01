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

// 包含binoffset校准和cg校准
DTOF_RET dtof_do_xtalk_calibration(dtof_uint8_t device_id, dtof_uint16_t* ft_data)
{
    DTOF_RET ret;
    dtof_uint16_t bin_offset;
    dtof_uint16_t ct_reg_data[DTOF_CT_REG_NUM];

    // close lp mode
    dtof_write_reg_running_lib(device_id, 0xca, 0x5588);
    // bypass
    dtof_set_mcu_status(device_id, DTOF_MCU_STATE_SLEEP_DIRECT);

    DTOF_CHECK_RET(dtof_calibration_refbinoffset_calculate(device_id, &bin_offset), "bin offset calculate failed\n");

    DTOF_CHECK_RET(dtof_do_cross_talk_calibration(device_id, ct_reg_data), "cross talk calibration failed\n");

    // wakeup
    dtof_set_mcu_status(device_id, DTOF_MCU_STATE_WAKEUP);

    memcpy(ft_data, ct_reg_data, DTOF_CT_REG_NUM * sizeof(dtof_uint16_t));
    *(ft_data + DTOF_CT_REG_NUM) = bin_offset;

    return DTOF_RET_SUCCESS;
}