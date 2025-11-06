#include <math.h>
#include "inc/dtof_base_type.h"
#include "inc/dtof_log.h"
#include "inc/dtof_common.h"
#include "inc/dtof_api.h"
#include "inc/dtof_calibration_ft.h"
#include "inc/dtof_driver.h"
#include "inc/dtof_global_config.h"

DTOF_RET dtof_do_ft_calibration(dtof_uint16_t otp_ref_spad_mask, dtof_uint16_t distance, dtof_uint16_t is_to_sky)
{
    DTOF_RET ret = DTOF_RET_SUCCESS;
    dtof_ft_cali_param_t ft_cali_param;
    dtof_calibrate_data_ft_t cal_data;
    dtof_uint16_t ft_cali_type = dtof_get_ft_calibration_type();

    DTOF_CHECK_RET(dtof_start_ft_calibrate(), "ft start failed\n");
    DTOF_CHECK_RET(dtof_set_mcu_status_ram(DTOF_MCU_STATE_SLEEP_DIRECT), "mcu sleep failed\n");

    if(DTOF_BIT_GET(ft_cali_type, DTOF_FT_CALIBRATE_BINOFFSET))
    {
       // ret = dtof_calibration_refbinoffset_calculate(&cal_data.binoffset_cal_data);
        DTOF_CHECK_RET(ret, "ft binoffset cal failed\n");
    }

    if(DTOF_BIT_GET(ft_cali_type, DTOF_FT_CALIBRATE_REFSPAD))
    {
        dtof_spad_info_t spad_info[DTOF_REF_SPAD_MAX_NUM]; // refspad
        cal_data.ref_spad_cal.otp_ref_spad_mask = otp_ref_spad_mask;
        // ret = dtof_ref_spad_calibrate(
        //     cal_data.ref_spad_cal.otp_ref_spad_mask,
        //     &cal_data.ref_spad_cal.ref_spad,
        //     spad_info);
        DTOF_CHECK_RET(ret, "ft refspad cal failed\n");
        ft_cali_param.dtof_ft_data.ref_spad = cal_data.ref_spad_cal.ref_spad;
    }

    if(DTOF_BIT_GET(ft_cali_type, DTOF_FT_CALIBRATE_BINOFFSET))
    {
        // ret = dtof_calibration_refbinoffset_calculate(&cal_data.binoffset_cal_data);
        DTOF_CHECK_RET(ret, "ft binoffset cal failed\n");
        ft_cali_param.dtof_ft_data.bin_offset = cal_data.binoffset_cal_data.binoffset;
    }

    if(DTOF_BIT_GET(ft_cali_type, DTOF_FT_CALIBRATE_CG))
    {
        // 为 1 表示对空做校准
       // ret = dtof_do_cross_talk_calibration(&cal_data.cross_talk_data, is_to_sky);
        DTOF_CHECK_RET(ret, "ft ct cal failed\n");

        dtof_uint16_t *dtof_ct_data_p = (dtof_uint16_t*)&cal_data.cross_talk_data;
        for (dtof_uint32_t i = 0; i < (DTOF_AC_NUM + 1); i++)
        {
            ft_cali_param.dtof_ft_data.cg_data[i * 2] = *(dtof_ct_data_p + i) & 0xff;
            ft_cali_param.dtof_ft_data.cg_data[i * 2 + 1] = (*(dtof_ct_data_p + i) >> 8) & 0xff;
        }
    }

    if(DTOF_BIT_GET(ft_cali_type, DTOF_FT_CALIBRATE_B))
    {
        // 读取otp中原有的b值
        #define DTOF_FT_DATA_START 0x2000
        #define DTOF_FT_DATA_B_OFFSET 5

        dtof_uint16_t dtof_ft_data_start = DTOF_FT_DATA_START + dtof_get_chip_config()->version_lenth - DTOF_FT_DATA_B_OFFSET;
        dtof_int16_t dtof_b_value_otp;

        DTOF_CHECK_RET(dtof_reg_burst_write(DTOF_WRITE_RAM_START_REG_ADDR, &dtof_ft_data_start, 1), "write ram start addr failed\n");
        DTOF_CHECK_RET(dtof_reg_burst_read(DTOF_READ_RAM_START_REG_ADDR, (dtof_uint16_t*)&dtof_b_value_otp, 1), "read ft data failed\n");

        DTOF_CHECK_RET(dtof_set_mcu_status_ram(DTOF_MCU_STATE_WAKEUP), "MCU wakeup failed");

        // 计算b和k
        cal_data.kb_data.far_distance = distance;
     //   ret = dtof_do_distance_calibration_b_use_sdk(cal_data.kb_data.far_distance, &cal_data.kb_data);
        DTOF_CHECK_RET(ret, "ft distance b cal failed\n");
        ft_cali_param.dtof_ft_data.distance_b = dtof_b_value_otp + (dtof_int16_t)roundf(cal_data.kb_data.b);
        ft_cali_param.dtof_ft_data.distance_k = (dtof_uint16_t)roundf(cal_data.kb_data.k * DTOF_FT_K_MULTIPLE);
    } else {
        DTOF_CHECK_RET(dtof_set_mcu_status_ram(DTOF_MCU_STATE_WAKEUP), "MCU wakeup failed");
    }

    ft_cali_param.ft_calibration_type = ~ft_cali_type;
    DTOF_CHECK_RET(dtof_set_ft_data((dtof_uint16_t*)&ft_cali_param), "set ft data failed\n");
    DTOF_CHECK_RET(dtof_set_ft_data_to_flash((dtof_uint16_t*)&ft_cali_param, sizeof(ft_cali_param) / sizeof(dtof_uint16_t)), "set ft data to flash failed\n");

    return DTOF_RET_SUCCESS;
}