/**
 * @file dtof_calibration_smudge.h
 * @author jiao.xu
 * @brief about chip smudge calibration
 * @version 1.0
 * @date 2022-08-11
 *
 * @copyright Copyright (c) 2022
 *
 */

#include "inc/calibration/dtof_calibration_smudge.h"
#include "inc/calibration/dtof_calibration_common.h"
#include "sdk/inc/dtof_float.h"
#include "sdk/inc/dtof_common.h"
#include "inc/dev/dtof_hal.h"
#include "inc/dtof_log.h"

typedef struct dtof_smudge_calib_
{
    dtof_int16_t cross_talk_calib_peak;      // cross talk calibrate peak(normalize
                                             // 65536),use for smudge calibrate
    dtof_uint16_t cross_talk_calib_nextbstr; // cross talk calibrate nextbstr
                                             // ratio,use for smudge calibrate
    dtof_uint16_t smudge_calib_nextbstr;     // smudge calibrate nextbstr ratio,use for
                                             // agc selfadjust
    dtof_int16_t smudge_moving_bin
        [DTOF_CT_BIN_SIZE]; // 去底噪归一化平滑处理后的cross
                               // talk 32bin
    dtof_uint16_t last_reg_nextDc;
    dtof_uint16_t freq_cnt;            // smudge校准频率控制计数
    dtof_bool_t cross_talk_calib_flag; // 是否已经crosstalk校准
} dtof_smudge_calib_t;

#ifdef DTOF_MODULE_SMUDGE_CALIB
static dtof_smudge_calib_t dtof_smudge;

dtof_smudge_calib_t *get_smudge_dev_p(void) { return &dtof_smudge; }

/**
 * @brief do smudge calibrate moving average.
 * @param new_val: input new in data(base on 65536).
 * @param average: output moving average data(base on 65536).
 * @param size: input data size.
 * @retval See the details in dtof_errno.h.
 */
static DTOF_RET dtof_smudge_calibrate_moving_average(const dtof_int16_t *new_val,
                                                        dtof_int16_t *average,
                                                        dtof_uint16_t size)
{
    DTOF_CHECK_PARAM(new_val && average && (size == DTOF_CT_BIN_SIZE),
                "移动平均参数错误");

    dtof_uint16_t clv_maxinx = 0;
    dtof_uint16_t moving_maxinx = 0;
    dtof_uint16_t peak_idx = 0;
    dtof_int16_t mean0, mean1;
    dtof_int16_t clv_sig_lpf = 0;
    dtof_int16_t moving_sig_lpf = 0;
    dtof_real32_t diff_sum;

    // 查找最大值索引
    DTOF_CHECK_RET(dtof_find_max_uint16(new_val, size, &clv_maxinx),
              "查找新数据最大值失败");

    // 边界检查和峰值计算
    peak_idx = (clv_maxinx < DTOF_SMUDGE_PEAK_IDX) ? 0 :
               (clv_maxinx > (size - DTOF_SMUDGE_PEAK_LEN + DTOF_SMUDGE_PEAK_IDX)) ?
               (size - DTOF_SMUDGE_PEAK_LEN) :
               (clv_maxinx - DTOF_SMUDGE_PEAK_IDX);

    // 计算新数据信号值
    DTOF_CHECK_RET(dtof_calc_mean_int16(&new_val[peak_idx],
                                  DTOF_SMUDGE_PEAK_LEN, &mean0),
              "计算新数据平均值失败");
    DTOF_CHECK_RET(dtof_calc_mean_int16(&new_val[DTOF_SMUDGE_NOISE_IDX],
                                  DTOF_SMUDGE_NOISE_LEN, &mean1),
              "计算新数据噪声值失败");
    clv_sig_lpf = mean0 - mean1;

    // 计算历史数据信号值
    DTOF_CHECK_RET(dtof_find_max_uint16(average, size, &moving_maxinx),
              "查找历史数据最大值失败");

    peak_idx = (moving_maxinx < DTOF_SMUDGE_PEAK_IDX) ? 0 :
               (moving_maxinx > (size - DTOF_SMUDGE_PEAK_LEN + DTOF_SMUDGE_PEAK_IDX)) ?
               (size - DTOF_SMUDGE_PEAK_LEN) :
               (moving_maxinx - DTOF_SMUDGE_PEAK_IDX);

    DTOF_CHECK_RET(dtof_calc_mean_int16(&average[peak_idx],
                                  DTOF_SMUDGE_PEAK_LEN, &mean0),
              "计算历史数据平均值失败");
    DTOF_CHECK_RET(dtof_calc_mean_int16(&average[DTOF_SMUDGE_NOISE_IDX],
                                  DTOF_SMUDGE_NOISE_LEN, &mean1),
              "计算历史数据噪声值失败");
    moving_sig_lpf = mean0 - mean1;

    // 计算差异比率
    if (clv_sig_lpf >= moving_sig_lpf) {
        diff_sum = (moving_sig_lpf == 0) ?
                  dtof_from_float(DTOF_SMUDGE_DIFF_TH) :
                  dtof_from_float(clv_sig_lpf * 1.0f / moving_sig_lpf);
    } else {
        diff_sum = (clv_sig_lpf == 0) ?
                  dtof_from_float(DTOF_SMUDGE_DIFF_TH) :
                  dtof_from_float(moving_sig_lpf * 1.0f / clv_sig_lpf);
    }

    // 更新移动平均
    if (diff_sum >= dtof_from_float(DTOF_SMUDGE_DIFF_TH)) {
        memcpy(average, new_val, size * sizeof(dtof_int16_t));
    } else {
        for (dtof_uint16_t i = 0; i < size; i++) {
            average[i] = (dtof_int16_t)((average[i] * (DTOF_SMUDGE_MOVING_N - 1) /
                                        DTOF_SMUDGE_MOVING_N) +
                                       (new_val[i] / DTOF_SMUDGE_MOVING_N));
        }
    }

    return DTOF_RET_SUCCESS;
}

/**
 * @brief Get Smudg elimination method fixed param:ground noise.
 * @param cross_talk_hist: input cross talk histogram.
 * @param size: input cross talk histogram size.
 * @param noise: output ground noise.
 * @retval See the details in dtof_errno.h.
 */
static DTOF_RET dtof_smudge_calibrate_get_noise_amb(
    const dtof_uint16_t *cross_talk_hist, dtof_uint32_t size, dtof_uint16_t *noise)
{
    dtof_uint32_t sum;

    if (!cross_talk_hist || (DTOF_CT_BIN_SIZE != size) || !noise)
    {
        return DTOF_RET_ERROR;
    }
    sum = (cross_talk_hist[NOISE_AMB_SELECT_INDEX0] +
           cross_talk_hist[NOISE_AMB_SELECT_INDEX1] +
           cross_talk_hist[NOISE_AMB_SELECT_INDEX2] +
           cross_talk_hist[NOISE_AMB_SELECT_INDEX3] +
           cross_talk_hist[NOISE_AMB_SELECT_INDEX4] +
           cross_talk_hist[NOISE_AMB_SELECT_INDEX5]);
    *noise = (dtof_uint16_t)(sum / NOISE_AMB_MAX_INDEX);
    return DTOF_RET_SUCCESS;
}

/**
 * @brief cross talk histogram normalize(base on 65536).
 * @param cross_talk_hist: input Real-time cross talk histogram.
 * @param cross_talk_normalize_hist: output cross talk normalize histogram.
 * @param size: input cross talk histogram bin size.
 * @param agc_coef: input Real-time AGC coefficient.
 * @retval See the details in dtof_errno.h.
 */
static DTOF_RET dtof_smudge_calibrate_cross_talk_histogram_normalize(
    const dtof_uint16_t *cross_talk_hist, dtof_int16_t *cross_talk_normalize_hist,
    dtof_uint16_t size, dtof_real32_t agc_coef)
{
    // dtof_int32_t sum;
    dtof_uint16_t noise_amb;
    dtof_uint16_t i;
    DTOF_RET ret;

    if (!cross_talk_hist || !cross_talk_normalize_hist ||
        (DTOF_CT_BIN_SIZE != size) ||
        ((dtof_from_float(DTOF_EPSINON) >= agc_coef) && (dtof_from_float(-DTOF_EPSINON) <= agc_coef)))
    {
        DTOF_LOG("paramter error!\n");
        return DTOF_RET_ERROR;
    }

    // calculate ground noise
    ret = dtof_smudge_calibrate_get_noise_amb(
        cross_talk_hist, DTOF_CT_BIN_SIZE, &noise_amb);
    if (DTOF_RET_SUCCESS != ret)
    {
        return ret;
    }

    for (i = 0; i < size; i++)
    {
        // Data normalization(base on 65536)
        cross_talk_normalize_hist[i] =
            (dtof_int16_t)(dtof_to_int(dtof_div(dtof_from_int(cross_talk_hist[i] - noise_amb), agc_coef)));
    }
    // Filter out bin0(Bin0 is use for special function in chip)
    cross_talk_normalize_hist[0] = 0;

    return DTOF_RET_SUCCESS;
}

/**
 * @brief smudge calibrate calculate.
 * @param deviceID: device id.
 * @param cross_talk_hist: input Real-time cross talk histogram.
 * @param agc_coef: input Real-time AGC coefficient.
 * @param nextbstr_reg: output register 0x90 value.
 * @retval See the details in dtof_errno.h.
 */
static DTOF_RET dtof_smudge_calibrate_calculate(const dtof_uint16_t *cross_talk_hist,
                                                   dtof_real32_t agc_coef,
                                                   dtof_uint16_t *nextbstr_reg)
{
    dtof_uint16_t maxInx = 0;
    dtof_int16_t Peak_v = 0;
    dtof_int16_t cross_talk_normalize_hist[DTOF_CT_BIN_SIZE] = {0};
    DTOF_RET ret;
    dtof_smudge_calib_t *pSmudge = get_smudge_dev_p();

    if (!pSmudge || !cross_talk_hist ||
        ((dtof_from_float(DTOF_EPSINON) >= agc_coef) && (dtof_from_float(-DTOF_EPSINON) <= agc_coef)) ||
        !nextbstr_reg)
    {
        DTOF_LOG("paramter error!\n");
        return DTOF_RET_ERROR;
    }

    // 01:Data normalization(base on 65536)
    ret = dtof_smudge_calibrate_cross_talk_histogram_normalize(
        cross_talk_hist, cross_talk_normalize_hist, DTOF_CT_BIN_SIZE,
        agc_coef);
    if (DTOF_RET_SUCCESS != ret)
    {
        return ret;
    }

    // 02:smoothing filtering process
    ret = dtof_smudge_calibrate_moving_average(cross_talk_normalize_hist,
                                               pSmudge->smudge_moving_bin,
                                               DTOF_CT_BIN_SIZE);
    if (DTOF_RET_SUCCESS != ret)
    {
        return ret;
    }

    // 03:find peak of Real-time cross talk histogram(normalize 65536)
    ret = dtof_find_max_uint16(pSmudge->smudge_moving_bin,
                              DTOF_CT_BIN_SIZE, &maxInx);
    if (DTOF_RET_SUCCESS != ret)
    {
        return ret;
    }
    Peak_v = pSmudge->smudge_moving_bin[maxInx];

    // 04:calculate ratio
    // Peak_v: peak count of smudge cross talk(normalize 65536)
    // cross_talk_calib_peak:cross talk calibration peak count(normalize 65536)
    // cross_talk_calib_nextbstr:cross talk ratio of cross talk Calibration
    // normalized process smudge_calib_nextbstr:cross talk ratio after Smudge
    // Calibration normalized process
    if (0 == pSmudge->cross_talk_calib_peak)
    {
        pSmudge->smudge_calib_nextbstr = 1;
    }
    else
    {
        pSmudge->smudge_calib_nextbstr =
            (dtof_uint16_t)(Peak_v * pSmudge->cross_talk_calib_nextbstr *
                            DTOF_VAL_BOOST_MULTIPLE / pSmudge->cross_talk_calib_peak);
    }

    // round up
    pSmudge->smudge_calib_nextbstr = ((dtof_uint16_t)(value_ceil(
        pSmudge->smudge_calib_nextbstr, DTOF_VAL_BOOST_MULTIPLE)));

    // *nextbstr_reg:calculate 0x90 register
    // agc_coef:agc coefficient
    *nextbstr_reg = ((dtof_uint16_t)(dtof_mul(dtof_from_int(pSmudge->smudge_calib_nextbstr *
                                                                   DTOF_VAL_BOOST_MULTIPLE),
                                                 agc_coef)));

    // round up
    *nextbstr_reg =
        ((dtof_uint16_t)(value_ceil(*nextbstr_reg, DTOF_VAL_BOOST_MULTIPLE)));

    DTOF_LOG(
        "do smudge:reg90=%d new_nextbstr_ratio=%d cross_talk_calib_nextbstr=%d "
        "Peak_v=%d cross_talk_calib_peak=%d agc_coef=%f \n",
        *nextbstr_reg, pSmudge->smudge_calib_nextbstr,
        pSmudge->cross_talk_calib_nextbstr, Peak_v,
        pSmudge->cross_talk_calib_peak, agc_coef);

    return DTOF_RET_SUCCESS;
}
/**
 * @brief Calculate agC coefficients.
 * @param flash_num: input flash number of FIFO.
 * @param agc_coef: output agC coefficients.
 * @retval See the details in dtof_errno.h.
 */
static DTOF_RET dtof_agc_coefficient_calculate(dtof_uint16_t flash_num,
                                                  dtof_real32_t *agc_coef)
{
    if (!agc_coef)
    {
        return DTOF_RET_ERROR;
    }
    *agc_coef = dtof_from_float(flash_num * 1.0f / DTOF_AGC_MAX_FLASH_NUM);
    return DTOF_RET_SUCCESS;
}
/**
 * @brief Calculate the AGC adaptive 0x90 value.
 * @param deviceID: device id.
 * @param agc_coef: input Real-time AGC coefficient.
 * @param nextbstr_reg: output register 0x90 value.
 * @retval See the details in dtof_errno.h.
 */
static DTOF_RET dtof_agc_self_adjust_calculate(dtof_real32_t agc_coef,
                                                  dtof_uint16_t *nextbstr_reg)
{
    dtof_smudge_calib_t *pSmudge = get_smudge_dev_p();

    if (!pSmudge || !nextbstr_reg)
    {
        DTOF_LOG("paramter error!\n");
        return DTOF_RET_ERROR;
    }

    *nextbstr_reg = ((dtof_uint16_t)(dtof_mul(dtof_from_int(pSmudge->smudge_calib_nextbstr *
        DTOF_VAL_BOOST_MULTIPLE),
                                                 agc_coef)));
    // round up
    *nextbstr_reg =
        ((dtof_uint16_t)(value_ceil(*nextbstr_reg, DTOF_VAL_BOOST_MULTIPLE)));

    DTOF_LOG("do agc self ajust:reg90=%d agc_coef=%f ne_xt_bstr=%d\n",
              *nextbstr_reg, agc_coef, pSmudge->smudge_calib_nextbstr);
    return DTOF_RET_SUCCESS;
}

DTOF_RET dtof_smudge_set_crosstalk_param(dtof_uint16_t *cross_talk_calib_data,
                                            dtof_uint16_t size,
                                            dtof_uint16_t cross_talk_nextbstr)
{
    dtof_uint16_t maxIndex;
    dtof_smudge_calib_t *pSmudge = get_smudge_dev_p();
    if (!pSmudge || !cross_talk_calib_data || (DTOF_CT_BIN_SIZE != size))
    {
        DTOF_LOG("param error\r\n");
        return DTOF_RET_ERROR;
    }

    // 初始化cross talk校准90倍率，smudge校准时计算使用
    pSmudge->cross_talk_calib_nextbstr = cross_talk_nextbstr;
    // 初始化smudge倍率，用于当mp0<40做agc自适应时计算90值
    pSmudge->smudge_calib_nextbstr = cross_talk_nextbstr;

    // 初始化cross talk校准时的波峰count值，smudge校准时计算使用
    dtof_find_max_uint16((dtof_int16_t *)cross_talk_calib_data, size, &maxIndex);
    pSmudge->cross_talk_calib_peak = cross_talk_calib_data[maxIndex];

    // 初始化smudge校准时对归一化的32个bin平滑滤波数据打底
    memcpy((void *)pSmudge->smudge_moving_bin, (void *)cross_talk_calib_data,
           size * sizeof(dtof_uint16_t));
    pSmudge->last_reg_nextDc = 0;
    pSmudge->freq_cnt = 0;
    pSmudge->cross_talk_calib_flag = DTOF_TRUE;

    return DTOF_RET_SUCCESS;
}
DTOF_RET dtof_smudge_clear_crosstalk_param(void)
{
    dtof_smudge_calib_t *pSmudge = get_smudge_dev_p();
    if (!pSmudge)
    {
        DTOF_LOG("param error\r\n");
        return DTOF_RET_ERROR;
    }
    pSmudge->cross_talk_calib_flag = DTOF_FALSE;
    return DTOF_RET_SUCCESS;
}

DTOF_RET dtof_smudge_calibrate_init(void)
{
    dtof_smudge_calib_t *pSmudge = get_smudge_dev_p();
    if (!pSmudge)
    {
        DTOF_LOG("param error\r\n");
        return DTOF_RET_ERROR;
    }
    pSmudge->last_reg_nextDc = 0;
    pSmudge->freq_cnt = 0;
    pSmudge->cross_talk_calib_flag = DTOF_FALSE;
    return DTOF_RET_SUCCESS;
}

DTOF_RET dtof_smudge_calibrate_deinit(void)
{
    // TBD 释放模块的缓存
    return DTOF_RET_SUCCESS;
}


DTOF_RET dtof_smudge_calibrate(const dtof_uint16_t *cross_talk_hist,
                                  dtof_uint16_t size, dtof_uint16_t flashn,
                                  dtof_uint16_t mp0)
{
    dtof_uint16_t reg_nextDc;
    dtof_real32_t agc_coef;
    DTOF_RET ret;
    dtof_smudge_calib_t *pSmudge = get_smudge_dev_p();

    if (!pSmudge || DTOF_CT_BIN_SIZE != size || !cross_talk_hist)
    {
        DTOF_LOG("param error\r\n");
        return DTOF_RET_ERROR;
    }

    // smudge校准之前需要先完成smudge初始化
    // smudge初始化依赖crosstalk校准参数
    if (DTOF_TRUE != pSmudge->cross_talk_calib_flag)
    {
        DTOF_LOG("No smudge process!Not set cross talk param in smudge module!\r\n");
        return DTOF_RET_SUCCESS;
    }
    ret = dtof_agc_coefficient_calculate(flashn, &agc_coef);
    if (DTOF_RET_SUCCESS != ret)
    {
        DTOF_LOG("agc coefficient calculate fail\r\n");
        return ret;
    }

    // 根据设定的频率执行smudge校准。不做smudge校准时执行agc自适应
    pSmudge->freq_cnt++;
    if (0 == (pSmudge->freq_cnt % SMUDGE_CALIB_FREQ))
    {
        if (SMUDGE_CALIB_OBJECT_LIMIT_DIS < mp0)
        {
            ret = dtof_smudge_calibrate_calculate(cross_talk_hist,
                                                  agc_coef, &reg_nextDc);
            if (DTOF_RET_SUCCESS != ret)
            {
                DTOF_LOG("smudge calibrate calculate fail\r\n");
                return ret;
            }
        }
        else
        {
            ret = dtof_agc_self_adjust_calculate(agc_coef, &reg_nextDc);
            if (DTOF_RET_SUCCESS != ret)
            {
                DTOF_LOG("agc self adjust calculate fail\r\n");
                return ret;
            }
        }
    }
    else
    {
        ret = dtof_agc_self_adjust_calculate(agc_coef, &reg_nextDc);
        if (DTOF_RET_SUCCESS != ret)
        {
            DTOF_LOG("agc self adjust calculate fail\r\n");
            return ret;
        }
    }
    if (reg_nextDc != pSmudge->last_reg_nextDc)
    {
        ret = hal_next_dc_data_config(reg_nextDc);
        if (DTOF_RET_SUCCESS != ret)
        {
            DTOF_LOG("config near end cross talk DC data failed\r\n");
            return ret;
        }
        pSmudge->last_reg_nextDc = reg_nextDc;
    }

    return DTOF_RET_SUCCESS;
}

#endif
