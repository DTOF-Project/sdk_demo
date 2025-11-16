#include <math.h>
#include "inc/dtof_base_type.h"
#include "inc/dtof_log.h"
#include "inc/dtof_common.h"
#include "inc/dtof_api.h"
#include "inc/dtof_calibration_ft.h"
#include "inc/dtof_driver.h"
#include "src/lib/dtof_ft.h"
DTOF_RET dtof_do_ft_calibration(dtof_uint16_t otp_ref_spad_mask, dtof_uint16_t distance, dtof_uint16_t is_to_sky)
{
    DTOF_RET ret = DTOF_RET_SUCCESS;
    dtof_ft_data_t ft_data;
    dtof_calibrate_data_ft_t cal_data;

    DTOF_CHECK_RET(dtof_start_ft_calibrate(), "ft start failed\n");
    DTOF_CHECK_RET(dtof_set_mcu_status_ram(DTOF_MCU_STATE_SLEEP_DIRECT), "mcu sleep failed\n");

#ifdef DTOF_FT_CALIBRATE_BINOFFSET
    ret = dtof_calibration_refbinoffset_calculate(&cal_data.binoffset_cal_data);
    DTOF_CHECK_RET(ret, "ft binoffset cal failed\n");
#endif

#ifdef DTOF_FT_CALIBRATE_REFSPAD
    dtof_spad_info_t spad_info[DTOF_REF_SPAD_MAX_NUM]; // refspad
    cal_data.ref_spad_cal.otp_ref_spad_mask = otp_ref_spad_mask;
    ret = dtof_ref_spad_calibrate(
        cal_data.ref_spad_cal.otp_ref_spad_mask,
        &cal_data.ref_spad_cal.ref_spad,
        spad_info);
    DTOF_CHECK_RET(ret, "ft refspad cal failed\n");
    ft_data.ref_spad = cal_data.ref_spad_cal.ref_spad;
#endif

#ifdef DTOF_FT_CALIBRATE_BINOFFSET
    ret = dtof_calibration_refbinoffset_calculate(&cal_data.binoffset_cal_data);
    DTOF_CHECK_RET(ret, "ft binoffset cal failed\n");
    ft_data.bin_offset = cal_data.binoffset_cal_data.binoffset;
#endif

#ifdef DTOF_FT_CALIBRATE_CG
    // 为 1 表示对空做校准
    //ret = dtof_do_cross_talk_calibration(&cal_data.cross_talk_data, is_to_sky);
    DTOF_CHECK_RET(ret, "ft ct cal failed\n");

    dtof_uint16_t *dtof_ct_data_p = (dtof_uint16_t*)&cal_data.cross_talk_data;
    for (dtof_uint32_t i = 0; i < (DTOF_AC_NUM + 1); i++)
    {
        ft_data.cg_data[i * 2] = *(dtof_ct_data_p + i) & 0xff;
        ft_data.cg_data[i * 2 + 1] = (*(dtof_ct_data_p + i) >> 8) & 0xff;
    }
#endif

#ifdef DTOF_FT_CALIBRATE_B
    cal_data.kb_data.far_distance = distance;
    ret = dtof_do_distance_calibration_b(cal_data.kb_data.far_distance, &cal_data.kb_data);
    DTOF_CHECK_RET(ret, "ft distance b cal failed\n");
    ft_data.distance_b = (dtof_uint16_t)roundf(cal_data.kb_data.b);
    ft_data.distance_k = (dtof_uint16_t)roundf(cal_data.kb_data.k * DTOF_FT_K_MULTIPLE);
#endif

    DTOF_CHECK_RET(dtof_set_mcu_status_ram(DTOF_MCU_STATE_WAKEUP), "MCU sleep failed");
    DTOF_CHECK_RET(dtof_stop_distance_measure(), "stop failed");

    dtof_set_ft_data((dtof_uint16_t*)&ft_data);
    dtof_set_ft_data_to_flash((dtof_uint16_t*)&ft_data, sizeof(ft_data) / sizeof(dtof_uint16_t));

    return DTOF_RET_SUCCESS;
}