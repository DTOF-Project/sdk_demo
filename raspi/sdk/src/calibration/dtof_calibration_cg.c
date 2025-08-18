#include "inc/dtof_common.h"
#include "inc/dtof_log.h"
#include "inc/dev/dtof_hal.h"
#include "inc/calibration/dtof_calibration_cg.h"
#include "inc/calibration/dtof_calibration_common.h"
#include "inc/calibration/dtof_calibration.h"

// 无符号数减法, 结果不会小于0
#define SAFE_UNSIGNED_SUB(x, y) (((x) > (y)) ? ((x) - (y)) : 0)
#define SAFE_ADD_OR_SUB(x) ((x) > 0 ? (x) : 0)

int max3(int a, int b, int c) {
    int max = a;
    if (b > max) max = b;
    if (c > max) max = c;
    return max;
}

DTOF_RET dtof_cross_talk_sample_data(uint16_t frame_size, uint16_t *cross_talk_calib_data)
{
    uint16_t captured_frm_cnt = 0;
    uint32_t calib_histgram[DTOF_CT_BIN_SIZE_NEW] = {0};
    uint16_t maxfls_backup;
    uint32_t cg_front_mean = 0;
    uint16_t cg_center;
    uint16_t cg_left;
    uint16_t cg_right;
    uint16_t cg_find_max[DTOF_CT_BIN_SIZE_NEW];
    DTOF_RET ret = DTOF_RET_ERROR;

    // 参数检查
    if ((0 == frame_size) || !cross_talk_calib_data) {
        DTOF_LOG("串扰采样参数错误\r\n");
        return DTOF_RET_ERROR;
    }

    // 保存并配置最大闪光次数
    ret = hal_dtof_maxfls_get(&maxfls_backup);
    DTOF_CHECK_RET(ret, "获取最大闪光次数失败\n");

    ret = hal_dtof_maxfls_config(DTOF_CG_CALIB_FLASH_CNT);
    DTOF_CHECK_RET(ret, "配置最大闪光次数失败\n");

    // 采集直方图数据
    while (frame_size > captured_frm_cnt) {
        ret = dtof_calibration_get_frame_data(0, cross_talk_calib_data, DTOF_CT_BIN_SIZE_NEW);
        DTOF_CHECK_RET(ret, "获取帧数据失败\n");

        for (int32_t i = 0; i < DTOF_CT_BIN_SIZE_NEW; i++) {
            calib_histgram[i] += cross_talk_calib_data[i];
#ifdef DTOF_CT_DEBUG
            printf("%d, ", cross_talk_calib_data[i]);
#endif
        }
#ifdef DTOF_CT_DEBUG
        printf("\n");
#endif
        ++captured_frm_cnt;
    }

    // 计算平均值
    for (int i = 0; i < DTOF_CT_BIN_SIZE_NEW; i++) {
        cross_talk_calib_data[i] = (calib_histgram[i] / frame_size);
    }

    for (int i = 0; i < DTOF_CT_FRONT_BIN_SIZE; i++)
    {
        cg_front_mean += cross_talk_calib_data[i];
    }
    cg_front_mean /= DTOF_CT_FRONT_BIN_SIZE;

    for (int i = 0; i < DTOF_CT_BIN_SIZE_NEW; i++)
    {
        cross_talk_calib_data[i] = SAFE_UNSIGNED_SUB(cross_talk_calib_data[i], cg_front_mean) * DTOF_VAL_SAMPLE_MULTIPLE / DTOF_VAL_BOOST_MULTIPLE;
    }

    memcpy(cg_find_max, cross_talk_calib_data, DTOF_CT_BIN_SIZE_NEW * sizeof(uint16_t));

    // 左移两位，后两位补 0
    for (int i = 0; i < DTOF_CT_BIN_SIZE_NEW; i++) {
        cg_center = cg_find_max[i];
        cg_left = ((i + 2) < DTOF_CT_BIN_SIZE_NEW) ? cg_find_max[i + 2] : cg_find_max[DTOF_CT_BIN_SIZE_NEW - 1];
        cg_right = (i >= 2) ? cg_find_max[i - 2] : cg_find_max[0];
        cross_talk_calib_data[i] = max3(cg_center, cg_left, cg_right);
    }

    // 恢复最大闪光次数
    ret = hal_dtof_maxfls_config(maxfls_backup);
    DTOF_CHECK_RET(ret, "恢复最大闪光次数失败\n");

    return ret;
}

DTOF_RET dtof_cross_talk_calibrate_calculate(const uint16_t *cross_talk_histgram, uint16_t* cross_talk_bin_data, uint16_t *cross_talk_ac_data, uint16_t *cross_talk_dc_data)
{
    DTOF_RET ret;
    uint16_t maxfls;
    int32_t i = 0;
    uint16_t temp_sample_data_v[DTOF_CT_BIN_SIZE_NEW];
    uint32_t tempdata_v[DTOF_CT_TEMP_SIZE] = {0};

    if ( !cross_talk_histgram || !cross_talk_ac_data || !cross_talk_dc_data )
    {
        DTOF_LOG("cross talk calculate param error\r\n");
        return DTOF_RET_ERROR;
    }

    // 保存最大闪光次数
    ret = hal_dtof_maxfls_get(&maxfls);
    if(ret != DTOF_RET_SUCCESS){
        DTOF_LOG("get max flash fail\n");
    }

    // 对采样数据做归一
    for (i = 0; i < DTOF_CT_BIN_SIZE_NEW; i++)
    {
        temp_sample_data_v[i] = cross_talk_histgram[i] * maxfls / (DTOF_CG_CALIB_FLASH_CNT + 1);
    }

    // 对65bin数据做32bin的滑动窗口处理
    for (i = 0; i < DTOF_CT_TEMP_SIZE; i++)
    {
        tempdata_v[i] = (temp_sample_data_v[i * 2] + temp_sample_data_v[i * 2 + 1] + temp_sample_data_v[i * 2 + 2]) / 3;
    }

    // save cg data
    for (i = 0; i < DTOF_CT_TEMP_SIZE; i++)
    {
        *(cross_talk_bin_data + i) = (uint16_t)tempdata_v[i];
    }

    // 计算DC值
    *cross_talk_dc_data = value_ceil(dtof_find_max_uint32(tempdata_v, DTOF_CT_TEMP_SIZE), 256);

    // 量化32bin数据
    for (i = 0; i < DTOF_CT_TEMP_SIZE; i++)
    {
        tempdata_v[i] = value_round(tempdata_v[i], *cross_talk_dc_data);
    }

    // 计算AC值
    for (i = 0; i < DTOF_CT_PARAM_SIZE; i++) {
        *(cross_talk_ac_data + i) = tempdata_v[i * 2 + 1] * 256 + tempdata_v[i * 2];
    }

    return DTOF_RET_SUCCESS;
}

DTOF_RET dtof_cross_talk_data_compensation(dtof_uint16_t* cross_talk_sample_data_p)
{
    for(int i = 0; i < DTOF_CT_CAL_SLOPE_BIN_SIZE; i++){
        cross_talk_sample_data_p[DTOF_CT_CAL_SLOPE_START_BIN + i] = SAFE_ADD_OR_SUB(cross_talk_sample_data_p[DTOF_CT_CAL_SLOPE_START_BIN - 1] + i * DTOF_CT_CAL_SLOPE + 0.5f); // 四舍五入
    }
    return DTOF_RET_SUCCESS;
}

DTOF_RET dtof_do_cross_talk_calibration(int32_t type, cross_talk_data_t *cross_talk_data_p)
{
    DTOF_CHECK_PARAM(cross_talk_data_p, "串扰校准参数为空");

    // 采样串扰数据
    DTOF_CHECK_RET(dtof_cross_talk_sample_data(DTOF_CG_CALIB_FRAME_SIZE, cross_talk_data_p->cross_talk_sample_data),
                    "串扰数据采样失败");

    if(type == DTOF_CALIBRATION_CROSS_TALK_TO_OBJECT){
        DTOF_CHECK_RET(dtof_cross_talk_data_compensation(cross_talk_data_p->cross_talk_sample_data),
                    "数据补偿失败");
    }

    // 计算串扰校准参数
    DTOF_CHECK_RET(dtof_cross_talk_calibrate_calculate(
                    cross_talk_data_p->cross_talk_sample_data,
                    cross_talk_data_p->cross_talk_bin_data,
                    cross_talk_data_p->next_ac,
                    &cross_talk_data_p->next_dc),
                    "串扰校准计算失败");

    // 更新串扰校准寄存器
    DTOF_CHECK_RET(hal_next_ac_data_config(cross_talk_data_p->next_ac, DTOF_CT_PARAM_SIZE), "配置next_ac寄存器失败");
    DTOF_CHECK_RET(hal_next_dc_data_config(cross_talk_data_p->next_dc), "配置next_dc寄存器失败");

    return DTOF_RET_SUCCESS;
}
