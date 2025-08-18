/**
 * @file dtof_calibration.c
 * @author jiao.xu
 * @brief DTOF校准功能实现
 * @version 1.0
 * @date 2022-08-11
 *
 * @copyright Copyright (c) 2022
 */

#include "inc/dtof_base_type.h"
#include "inc/dtof_log.h"
#include "inc/dtof_api.h"
#include "inc/dtof_global_config.h"
#include "inc/dev/dtof_hal.h"
#include "inc/calibration/dtof_calibration_cg.h"
#include "inc/calibration/dtof_calibration_common.h"
#include "inc/calibration/dtof_calibration.h"
#include "inc/calibration/dtof_refbinoffset.h"
#include "inc/calibration/dtof_calibration_reference_spad.h"
#include "inc/calibration/dtof_bin_width_calibrate.h"
#include "inc/dev/dtof_dev_api.h"

static dtof_calibrate_data_t g_calibrate_param;

dtof_calibrate_data_t* get_calib_param(void) {
    return &g_calibrate_param;
}

DTOF_RET dtof_do_calibration(
    dtof_calibrate_type_t type,
    dtof_uint16_t actual_dis,
    dtof_calibrate_data_t *calibrate_data_p)
{
    DTOF_RET ret = DTOF_RET_SUCCESS;
    dtof_calibrate_data_t *calib_param = get_calib_param();
    dtof_bool_t need_update = DTOF_FALSE;

    DTOF_CHECK_PARAM(calibrate_data_p, "校准参数为空");

    // 通过ram code关闭软件低功耗
    #define FRAME_CONTROL_REG   0xca
    #define STOP_DISATNCE_MODE  0X6688            // 默认是stop状态, 此时处于软件低功耗模式, 且内部wfi
    #define FT_TEST_START_MODE  0X5588            // FT测试之前, 关闭软件低功耗模式, 且内部wfi

#ifdef DTOF_L3
    DTOF_CHECK_RET(dtof_write_reg_running(FRAME_CONTROL_REG, FT_TEST_START_MODE),
                    "关闭软件低功耗失败");

    // 设置MCU为间接睡眠模式
    DTOF_CHECK_RET(dtof_set_mcu_status(DTOF_CMD_MCU_SLEEP_DIR),
                    "MCU模式设置失败");
#endif
    DTOF_LOG("开始校准, 类型 = %d\n", type);

    // 根据校准类型执行相应的校准
    switch (type) {
        case DTOF_CALIBRATION_CROSS_TALK_TO_SKY:
        case DTOF_CALIBRATION_CROSS_TALK_TO_OBJECT:
            DTOF_CHECK_RET(dtof_do_cross_talk_calibration(type, &calib_param->ct_data),
                     "串扰校准失败");
            calib_param->valid_flag.crosstalk_valid = 1;
            need_update = DTOF_TRUE;
            break;

        case DTOF_CALIBRATION_REF_SPAD:
            DTOF_CHECK_RET(dtof_ref_spad_calibrate(
                         actual_dis,
                         &calib_param->rs_data.spad_mask, calib_param->rs_data.spad_info),
                     "参考SPAD校准失败");
            calib_param->valid_flag.refspad_valid = 1;
            need_update = DTOF_TRUE;
            DTOF_LOG("参考SPAD校准成功: 0x%x\n",
                     calib_param->rs_data.spad_mask);
            break;

        case DTOF_CALIBRATION_REF_BINOFFSET:
        {
            ret = dtof_calibration_refbinoffset_calculate(&calib_param->binoffset_data);
            if (DTOF_RET_SUCCESS != ret) {
                DTOF_CHECK_RET(DTOF_RET_ERROR, "binoffset校准失败");
            }
            calib_param->valid_flag.offset_valid = 1;
            DTOF_LOG("binoffset calibration: %d\n", calib_param->binoffset_data.binoffset);
            break;
        }
        case DTOF_CALIBRATION_DOUBLE_POINT_CG:
        {
            ret = dtof_do_distance_calibration(actual_dis, &calib_param->distance_data);
            if (DTOF_RET_SUCCESS != ret) {
                DTOF_CHECK_RET(DTOF_RET_ERROR, "cg峰kb校准失败");
            }
            calib_param->valid_flag.fardis_valid = 1;
            DTOF_LOG("distance calibration: k: %f, b: %f\n", calib_param->distance_data.k, calib_param->distance_data.b);
            break;
        }
        case DTOF_CALIBRATION_DOUBLE_POINT_FAR:
        {
            ret = dtof_do_distance_calibration_old(actual_dis, type, calib_param);
            if (DTOF_RET_SUCCESS != ret) {
                DTOF_CHECK_RET(DTOF_RET_ERROR, "远端校准失败");
            }
            calib_param->valid_flag.fardis_valid = 1;
            if(calib_param->valid_flag.neardis_valid == 1)
            {
                DTOF_LOG("distance calibration: k: %f, b: %f\n", calib_param->distance_data.k, calib_param->distance_data.b);
            }
            break;
        }
        case DTOF_CALIBRATION_DOUBLE_POINT_NEAR:
        {
            ret = dtof_do_distance_calibration_old(actual_dis, type, calib_param);
            if (DTOF_RET_SUCCESS != ret) {
                DTOF_CHECK_RET(DTOF_RET_ERROR, "近端校准失败");
            }
            calib_param->valid_flag.neardis_valid = 1;
            if(calib_param->valid_flag.fardis_valid == 1)
            {
                DTOF_LOG("distance calibration: k: %f, b: %f\n", calib_param->distance_data.k, calib_param->distance_data.b);
            }
            break;
        }
        case DTOF_CALIBRATION_SINGLE_POINT:
        {
            ret = dtof_do_distance_calibration_b(actual_dis, type, calib_param);
            if (DTOF_RET_SUCCESS != ret) {
                DTOF_CHECK_RET(DTOF_RET_ERROR, "b值校准失败");
            }
            DTOF_LOG("distance calibration: k: %f, b: %f\n", calib_param->distance_data.k, calib_param->distance_data.b);
            break;
        }
        case DTOF_CALIBRATION_PERFORMANCE:
        {
            ret = dtof_performance_verify(&calib_param->performance);
            if (DTOF_RET_SUCCESS != ret) {
                DTOF_CHECK_RET(DTOF_RET_ERROR, "性能校准失败");
            }
            break;
        }
        default:
            DTOF_LOG("未知的校准类型: %d\n", type);
            return DTOF_RET_ERROR;
    }

    // 更新校准参数
    if (ret == DTOF_RET_SUCCESS) {
        calib_param->version = PARAMETER_VERSION;
        memcpy(calibrate_data_p, calib_param, sizeof(dtof_calibrate_data_t));

        if (need_update) {
            // TODO: 更新传感器配置
        }
    }
#ifdef DTOF_L3
    // 唤醒MCU
    DTOF_CHECK_RET(dtof_set_mcu_status(DTOF_MCU_STATE_WAKEUP), "MCU唤醒失败");

    DTOF_CHECK_RET(dtof_write_reg_running(FRAME_CONTROL_REG, STOP_DISATNCE_MODE),
                    "开启软件低功耗失败");
#endif
    return ret;
}
