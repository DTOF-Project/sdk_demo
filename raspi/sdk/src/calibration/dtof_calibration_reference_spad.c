/**
 * @file dtof_calibration_reference_spad.h
 * @author jiao.xu
 * @brief about chip reference spad calibration
 * @version 1.0
 * @date 2022-08-11
 *
 * @copyright Copyright (c) 2022
 *
 */

#include "inc/calibration/dtof_calibration_common.h"
#include "inc/calibration/dtof_calibration_reference_spad.h"
#include "inc/dtof_driver.h"
#include "inc/dev/dtof_hal.h"
#include "inc/dtof_log.h"

/**
 * @brief  Init ref spad calibrate module.
 * @param  None.
 * @retval See the details in dtof_errno.h.
 *
 *  1. init ref spad calibrate module.
 */
DTOF_RET dtof_ref_spad_calibrate_init(void)
{

    return DTOF_RET_SUCCESS;
}
/**
 * @brief  Deinit ref spad calibrate module.
 * @param  None.
 * @retval See the details in dtof_errno.h.
 *
 *  1. TBD 释放模块的缓存.
 */

DTOF_RET dtof_ref_spad_calibrate_deinit(void)
{
    // TBD 根据deviceID释放模块的缓存
    return DTOF_RET_SUCCESS;
}

/**
 * @brief  refspad校准初始化.
 * @param  None.
 * @retval See the details in dtof_errno.h.
 *
 *  1. close all ref spad.
 */
static DTOF_RET dtof_ref_spad_cal_initialization(void)
{
    dtof_uint16_t close_all_spad = 0;
    DTOF_RET ret = DTOF_RET_SUCCESS;
    ret = hal_ref_spad_mask_config(close_all_spad);
    if (DTOF_RET_SUCCESS != ret) {
        return ret;
    }
    return DTOF_RET_SUCCESS;
}

static DTOF_RET sort_spad_info_array(dtof_spad_info_t *p_info, dtof_uint16_t len)
{
    if (!p_info || len == 0) {
        DTOF_LOG("参数错误\r\n");
        return DTOF_RET_ERROR;
    }

    // 使用插入排序替代冒泡排序，提高性能
    for (dtof_uint16_t i = 1; i < len; i++) {
        dtof_spad_info_t key = p_info[i];
        int j = i - 1;
        while (j >= 0 && p_info[j].spad_value > key.spad_value) {
            p_info[j + 1] = p_info[j];
            j--;
        }
        p_info[j + 1] = key;
    }
    return DTOF_RET_SUCCESS;
}
static DTOF_RET dtof_get_spad_info(dtof_uint16_t otp_spad_mask, dtof_spad_info_t *spad_info)
{
    if (!spad_info) {
        DTOF_LOG("SPAD信息指针为空\n");
        return DTOF_RET_ERROR;
    }

    if (otp_spad_mask == 0) {
        DTOF_LOG("无效的SPAD掩码\n");
        return DTOF_RET_ERROR;
    }

    for (dtof_uint16_t x = 0; x < DTOF_REF_SPAD_MAX_NUM; x++) {
        spad_info[x].spad_index = x;
        if (0 == (otp_spad_mask & (1u << ( x)))) {
            spad_info[x].spad_value = 0xFFFF;
            spad_info[x].is_valid = DTOF_FALSE;
        } else {
            DTOF_CHECK_RET(hal_ref_spad_mask_config((0x0001 << x)), "配置SPAD掩码");
            DTOF_CHECK_RET(hal_dtof_prepare_one_frame(), "准备帧");
            DTOF_CHECK_RET(dtof_histgram_io_read(DTOF_REF_SPAD_MV1_RAM_ADDR, &(spad_info[x].spad_value), 1), "读取MV1值");
            spad_info[x].is_valid = DTOF_TRUE;
        }
    }

    return DTOF_RET_SUCCESS;
}

static DTOF_RET dtof_ref_spad_calculate(dtof_spad_info_t *spad_info,
    dtof_uint16_t *ref_spad_cal) {

    if (!spad_info || !ref_spad_cal) {
        return DTOF_RET_ERROR;
    }

    dtof_uint32_t mv1_sum = 0;
    *ref_spad_cal = 0;
    // spad bit 获取方法
    for (dtof_uint16_t x = 0; x < DTOF_REF_SPAD_MAX_NUM; x++) {
        if (spad_info[x].is_valid == DTOF_TRUE) {
            if (mv1_sum + spad_info[x].spad_value < DTOF_REF_SPAD_MV1_SUM_MAX) {
                mv1_sum += spad_info[x].spad_value;
                *ref_spad_cal |= 1u << (spad_info[x].spad_index);
            } else {
                break;
            }
        }
    }

    return DTOF_RET_SUCCESS;
}

/**
 * @brief  ref spad calibration.
 * @param  otp_spad_mask: need to be calibrated ref spad mask.
 * @param  dc: ref spad 低16bit.
 * @retval See the details in dtof_errno.h.
 *
 *  1. close all ref spad.
 *  2. open a spad at one time.
 *  3. sort.
 *  4. select spad.
 *  5. update register after calibrating.
 */
DTOF_RET dtof_ref_spad_calibrate(dtof_uint16_t otp_ref_spad_mask, dtof_uint16_t *ref_spad_cal, dtof_spad_info_t *spad_info)
{
    DTOF_RET ret = DTOF_RET_SUCCESS;

    if (!otp_ref_spad_mask || !ref_spad_cal)
    {
        DTOF_LOG("param error\r\n");
        return DTOF_RET_ERROR;
    }

    // 1. do initialization ,close all ref spad
    ret = dtof_ref_spad_cal_initialization();
    if (DTOF_RET_SUCCESS != ret)
    {
        DTOF_LOG("ref spad cal initialization fail\r\n");
        return ret;
    }

    // otp mask 需要看情况, 客户说的算, 目前定位0xfe
    DTOF_LOG("otp_ref_spad_mask = 0x%08X\n", otp_ref_spad_mask);
    // 2. get spad info
    ret = dtof_get_spad_info(otp_ref_spad_mask, spad_info);
    if (DTOF_RET_SUCCESS != ret)
    {
        DTOF_LOG("get spad fail\r\n");
        return ret;
    }

    // 3. sort from smallest to largest
    ret = sort_spad_info_array(spad_info, DTOF_REF_SPAD_MAX_NUM);
    if (DTOF_RET_SUCCESS != ret)
    {
        DTOF_LOG("sort spade info array fail\r\n");
        return ret;
    }

    // 4. select spad
    ret = dtof_ref_spad_calculate(spad_info,ref_spad_cal);
    if (DTOF_RET_SUCCESS != ret)
    {
        DTOF_LOG("ref spad calculate fail\r\n");
        return ret;
    }

    // 5. config ref spad
    ret = hal_ref_spad_mask_config(*ref_spad_cal);
    if (DTOF_RET_SUCCESS != ret) {
        return ret;
    }

    return DTOF_RET_SUCCESS;
}
