#pragma GCC diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"

#include <stdint.h>
#include <math.h>
#include "inc/dtof_log.h"
#include "inc/dtof_base_type.h"
#include "inc/dtof_driver.h"
#include "inc/dtof_api.h"
#include "dtof_reg.h"
#include "dtof_hal.h"

// #define DTOF_CT_DEBUG

#define SAFE_ADD_OR_SUB(x) ((x) > 0 ? (x) : 0)

// 串扰校准相关参数定义
#define DTOF_CG_CALIB_FRAME_SIZE      10     // 采样帧数
#define DTOF_CG_CALIB_FLASH_CNT       0x3000 // Flash计数最大值

// 串扰校准相关参数定义
#define DTOF_CT_BIN_SIZE_NEW        65    // 新版本bin大小
#define DTOF_CT_SAMPLE_SIZE         3     // 采样大小
#define DTOF_CT_TEMP_SIZE          ((DTOF_CT_BIN_SIZE_NEW - 1) / 2)
#define DTOF_CT_PARAM_SIZE         (DTOF_CT_TEMP_SIZE / 2)
#define DTOF_CT_BIN_SIZE           32    // 标准bin大小
#define DTOF_CT_FRONT_BIN_SIZE     8     // 前端bin大小
#define DTOF_CG_PEAK_MAX_INDEX     26

// 数值处理相关参数
#define DTOF_VAL_BOOST_MULTIPLE    10    // 数值提升倍数，用于减少精度损失
#define DTOF_VAL_SAMPLE_MULTIPLE   12    // 采样数据*1.2倍再做CT
#define DTOF_VAL_SAMPLE_MULTIPLE_OBJECT   15
#define DTOF_VAL_JUDGE_CG_PEAK_MULTIPLE   15
#define DTOF_AGC_MAX_FLASH_NUM     65536 // AGC最大闪光次数

#define DTOF_RET_FIND_PEAK_ERROR        6
#define DTOF_RET_CG_PEAK_ERROR          7

#define DTOF_JUDGE_PEAK_BIN_SIZE 3
#define DTOF_FIND_MIN_PEAK_BIN_SIZE 15

typedef struct {
    dtof_uint16_t cross_talk_sample_data[DTOF_CT_BIN_SIZE_NEW];
    dtof_uint16_t cross_talk_bin_data[DTOF_CT_TEMP_SIZE]; // smudge use
    dtof_uint16_t next_ac[DTOF_CT_PARAM_SIZE];
    dtof_uint16_t next_dc;
} cross_talk_data_t;

int is_valid_peak(const uint16_t data[], int len, int peak_idx) {
    if ((peak_idx < DTOF_JUDGE_PEAK_BIN_SIZE) || (peak_idx >= len - DTOF_JUDGE_PEAK_BIN_SIZE - 1))
        return 0; // 边界不能是峰

    // 判断data[peak_idx]是否是峰
    for (int i = 1; i <= DTOF_JUDGE_PEAK_BIN_SIZE; i++) {
        if ((data[peak_idx - i] > data[peak_idx]) || (data[peak_idx + i] > data[peak_idx]))
            return 0; // 不是峰
    }

    return 1; // 是峰
}

uint32_t dtof_find_max_uint32(uint32_t *arr, int32_t size) {
    uint32_t max = *arr;

    for (int32_t i = 1; i < size; i++) {
        if (*(arr + i) > max) {
            max = *(arr + i);
        }
    }

    return max;
}

int max3(int a, int b, int c) {
    int max = a;
    if (b > max) max = b;
    if (c > max) max = c;
    return max;
}

DTOF_RET dtof_find_min_uint16(const uint16_t *buffer, uint16_t size, uint16_t *minIndex) {
    uint16_t i;
    uint16_t currMinIndex;
    uint16_t minValue;

    if (!buffer || (0 == size) || !minIndex) {
        return DTOF_RET_ERROR;
    }

    currMinIndex = 0;
    minValue = buffer[currMinIndex];
    for (i = 1; i < size; i++) {
        if (buffer[i] < minValue) {
            minValue = buffer[i];
            currMinIndex = i;
        }
    }
    *minIndex = currMinIndex;
    return DTOF_RET_SUCCESS;
}

uint32_t value_ceil(uint32_t x, uint32_t div) {
    if(div == 0){
        return 0;
    }
    return (x + div - 1) / div;
}

uint32_t value_round(uint32_t value, uint32_t multiple) {
    if (multiple == 0) {
        return 0;
    }

    uint32_t quotient = value / multiple;
    uint32_t remainder = value % multiple;

    if (remainder * 2 >= multiple) {
        quotient += 1;
    }

    return quotient;
}

DTOF_RET dtof_find_max_uint16(const uint16_t *buffer, uint16_t size, uint16_t *maxIndex) {
    uint16_t i;
    uint16_t currMaxIndex;
    int16_t maxValue;

    if (!buffer || (0 == size) || !maxIndex) {
        return DTOF_RET_ERROR;
    }

    currMaxIndex = 0;
    maxValue = buffer[currMaxIndex];
    for (i = 1; i < size; i++) {
        if (buffer[i] > maxValue) {
            maxValue = buffer[i];
            currMaxIndex = i;
        }
    }
    *maxIndex = currMaxIndex;
    return DTOF_RET_SUCCESS;
}

DTOF_RET dtof_cross_talk_sample_data(uint8_t device_id, uint16_t frame_size, uint16_t *cross_talk_calib_data)
{
    float slope;
    uint16_t captured_frm_cnt = 0;
    uint32_t calib_histgram[DTOF_CT_BIN_SIZE_NEW] = {0};
    uint16_t maxfls_backup;
    uint32_t cg_front_mean = 0;
    uint16_t cg_center;
    uint16_t cg_left;
    uint16_t cg_right;
    uint16_t cg_peak_pos;
    uint16_t object_peak_pos;
    uint16_t min_pos_between_two_peak;
    uint16_t cg_find_max[DTOF_CT_BIN_SIZE_NEW];
    int is_valid_peak_flag;
    DTOF_RET ret = DTOF_RET_ERROR;

    // 参数检查
    if ((0 == frame_size) || !cross_talk_calib_data) {
        DTOF_LOG("串扰采样参数错误\r\n");
        return DTOF_RET_ERROR;
    }

    // 保存并配置最大闪光次数
    ret = hal_dtof_maxfls_get(device_id, &maxfls_backup);
    DTOF_CHECK_RET(ret, "获取最大闪光次数失败\n");

    ret = hal_dtof_maxfls_config(device_id, DTOF_CG_CALIB_FLASH_CNT);
    DTOF_CHECK_RET(ret, "配置最大闪光次数失败\n");

    // 采集直方图数据
    while (frame_size > captured_frm_cnt) {
        ret = dtof_calibration_get_frame_data(device_id, 0, cross_talk_calib_data, DTOF_CT_BIN_SIZE_NEW);
        if (ret != DTOF_RET_SUCCESS) {
            // 恢复最大闪光次数
            DTOF_CHECK_RET(hal_dtof_maxfls_config(device_id, maxfls_backup), "恢复最大闪光次数失败\n");
            DTOF_LOG_ERR("dtof_calibration_get_frame_data fail\n");
            return ret;
        }

        for (int32_t i = 0; i < DTOF_CT_BIN_SIZE_NEW; i++) {
            calib_histgram[i] += cross_talk_calib_data[i];
        }
        ++captured_frm_cnt;
    }

#ifdef DTOF_CT_DEBUG
    printf("cross_talk_calib_data:\n");
#endif

    // 计算平均值
    for (int i = 0; i < DTOF_CT_BIN_SIZE_NEW; i++) {
        cross_talk_calib_data[i] = (calib_histgram[i] / frame_size);
        #ifdef DTOF_CT_DEBUG
            printf("%d, ", cross_talk_calib_data[i]);
        #endif
    }
#ifdef DTOF_CT_DEBUG
    printf("\n");
#endif
    for (int i = 0; i < DTOF_CT_FRONT_BIN_SIZE; i++)
    {
        cg_front_mean += cross_talk_calib_data[i];
    }
    cg_front_mean /= DTOF_CT_FRONT_BIN_SIZE;

    // for (int i = 0; i < DTOF_CT_BIN_SIZE_NEW; i++)
    // {
    //     cross_talk_calib_data[i] = SAFE_UNSIGNED_SUB(cross_talk_calib_data[i], cg_front_mean) * DTOF_VAL_SAMPLE_MULTIPLE / DTOF_VAL_BOOST_MULTIPLE;
    // }

    // dtof_find_max_uint16(cross_talk_calib_data, DTOF_CT_BIN_SIZE_NEW, &object_peak_pos);
    // dtof_find_max_uint16(cross_talk_calib_data, object_peak_pos - 8, &cg_peak_pos);

    for (int i = DTOF_CT_FRONT_BIN_SIZE; i < (DTOF_CT_BIN_SIZE_NEW - 3); i++)
    {
        is_valid_peak_flag = is_valid_peak(cross_talk_calib_data, DTOF_CT_BIN_SIZE_NEW, i);
        if(is_valid_peak_flag == 1) {
            if (cross_talk_calib_data[i] < value_ceil(cg_front_mean * DTOF_VAL_JUDGE_CG_PEAK_MULTIPLE, DTOF_VAL_BOOST_MULTIPLE)) { // 改为前八帧平均1.5倍做cg峰合法性校验
                continue; // cg peak must be larger than 1.5 of cg front mean
            }
            cg_peak_pos = i;
            break;
        }
    }

#ifdef DTOF_CT_DEBUG
    printf("cg_front_mean:%d\n", cg_front_mean);
    printf("object_peak_pos:%d\n", cg_peak_pos);
#endif

    if (is_valid_peak_flag != 1) {
        // 恢复最大闪光次数
        ret = hal_dtof_maxfls_config(device_id, maxfls_backup);
        DTOF_CHECK_RET(ret, "恢复最大闪光次数失败\n");
        DTOF_LOG_ERR("cant find cg peak!\n");
        return DTOF_RET_CG_PEAK_ERROR;
    }

    // if (object_peak_pos <= cg_peak_pos) {
    //     return DTOF_RET_FIND_PEAK_ERROR;
    // }

    for (int i = 0; i < DTOF_CT_BIN_SIZE_NEW; i++)
    {
        cross_talk_calib_data[i] = value_ceil(cross_talk_calib_data[i] * DTOF_VAL_SAMPLE_MULTIPLE_OBJECT, DTOF_VAL_BOOST_MULTIPLE);
    }

    dtof_find_min_uint16(cross_talk_calib_data + cg_peak_pos, DTOF_FIND_MIN_PEAK_BIN_SIZE, &min_pos_between_two_peak);
#ifdef DTOF_CT_DEBUG
    printf("min_pos_between_two_peak:%d\n", min_pos_between_two_peak);
#endif

    min_pos_between_two_peak += cg_peak_pos;
#ifdef DTOF_CT_DEBUG
    printf("min_pos_between_two_peak:%d\n", min_pos_between_two_peak);
#endif

    // 计算斜率
    slope = (float)(cross_talk_calib_data[min_pos_between_two_peak] - cg_front_mean) / (DTOF_CT_BIN_SIZE_NEW - min_pos_between_two_peak - 1);
    for (int i = min_pos_between_two_peak; i < (DTOF_CT_BIN_SIZE_NEW - 1); i++)
    {
        cross_talk_calib_data[i + 1] = (uint16_t)ceilf(SAFE_ADD_OR_SUB(cross_talk_calib_data[min_pos_between_two_peak] - slope * (i - min_pos_between_two_peak + 1)));
    }

#ifdef DTOF_CT_DEBUG
    printf("slope:%f\n", slope);
    printf("cross_talk_calib_data slope:\n");
    for (int i = 0; i < DTOF_CT_BIN_SIZE_NEW; i++)
    {
        printf("%d, ", cross_talk_calib_data[i]);
    }
    printf("\n");
#endif


    memcpy(cg_find_max, cross_talk_calib_data, DTOF_CT_BIN_SIZE_NEW * sizeof(uint16_t));

    // 左移两位，后两位补 0
    for (int i = 0; i < DTOF_CT_BIN_SIZE_NEW; i++) {
        cg_center = cg_find_max[i];
        cg_left = ((i + 1) < DTOF_CT_BIN_SIZE_NEW) ? cg_find_max[i + 1] : cg_find_max[DTOF_CT_BIN_SIZE_NEW - 1];
        cg_right = (i >= 1) ? cg_find_max[i - 1] : cg_find_max[0];
        cross_talk_calib_data[i] = max3(cg_center, cg_left, cg_right);
    }

#ifdef DTOF_CT_DEBUG
    printf("cross_talk_calib_data max:\n");
    for (int i = 0; i < DTOF_CT_BIN_SIZE_NEW; i++)
    {
        printf("%d, ", cross_talk_calib_data[i]);
    }
    printf("\n");
#endif

    // 恢复最大闪光次数
    ret = hal_dtof_maxfls_config(device_id, maxfls_backup);
    DTOF_CHECK_RET(ret, "恢复最大闪光次数失败\n");

    return ret;
}

DTOF_RET dtof_cross_talk_calibrate_calculate(uint8_t device_id, const uint16_t *cross_talk_histgram, uint16_t* cross_talk_bin_data, uint16_t *cross_talk_ac_data, uint16_t *cross_talk_dc_data)
{
    DTOF_RET ret;
    uint16_t maxfls;
    int32_t i = 0;
    // uint16_t temp_sample_data_v[DTOF_CT_BIN_SIZE_NEW];
    uint32_t tempdata_v[DTOF_CT_TEMP_SIZE] = {0};
    // dtof_uint16_t cross_talk_sample_data_temp[DTOF_CT_BIN_SIZE_NEW];

    if ( !cross_talk_histgram || !cross_talk_ac_data || !cross_talk_dc_data )
    {
        DTOF_LOG("cross talk calculate param error\r\n");
        return DTOF_RET_ERROR;
    }

    // 保存最大闪光次数
    ret = hal_dtof_maxfls_get(device_id, &maxfls);
    if(ret != DTOF_RET_SUCCESS){
        DTOF_LOG("get max flash fail\n");
    }

// #ifdef DTOF_CT_DEBUG
//     printf("cross_talk_calib_data normalization:\n");
// #endif
//     // 对采样数据做归一
//     for (i = 0; i < DTOF_CT_BIN_SIZE_NEW; i++)
//     {
//         cross_talk_sample_data_temp[i] = value_ceil(cross_talk_histgram[i] * maxfls, DTOF_CG_CALIB_FLASH_CNT);
//     #ifdef DTOF_CT_DEBUG
//         printf("%d, ", cross_talk_sample_data_temp[i]);
//     #endif
//     }
// #ifdef DTOF_CT_DEBUG
//     printf("\n");
// #endif

    // 对65bin数据做32bin的滑动窗口处理
    for (i = 0; i < DTOF_CT_TEMP_SIZE; i++)
    {
        tempdata_v[i] = (cross_talk_histgram[i * 2] + cross_talk_histgram[i * 2 + 1] + cross_talk_histgram[i * 2 + 2]) / 3;
    }

    // save cg data
#ifdef DTOF_CT_DEBUG
    printf("cross_talk_calib_data 32bin:\n");
#endif
    for (i = 0; i < DTOF_CT_TEMP_SIZE; i++)
    {
        *(cross_talk_bin_data + i) = (uint16_t)tempdata_v[i];
    #ifdef DTOF_CT_DEBUG
        printf("%d, ", *(cross_talk_bin_data + i));
    #endif
    }
#ifdef DTOF_CT_DEBUG
    printf("\n");
#endif

    // 计算DC值
    *cross_talk_dc_data = value_ceil(dtof_find_max_uint32(tempdata_v, DTOF_CT_TEMP_SIZE), 256);

    // 量化32bin数据
    for (i = 0; i < DTOF_CT_TEMP_SIZE; i++)
    {
        tempdata_v[i] = value_round(tempdata_v[i], *cross_talk_dc_data);
    }

#ifdef DTOF_CT_DEBUG
    printf("ac:\n");
#endif
    // 计算AC值
    for (i = 0; i < DTOF_CT_PARAM_SIZE; i++) {
        *(cross_talk_ac_data + i) = tempdata_v[i * 2 + 1] * 256 + tempdata_v[i * 2];
    #ifdef DTOF_CT_DEBUG
        printf("%d, ", *(cross_talk_ac_data + i));
    #endif
    }
#ifdef DTOF_CT_DEBUG
    printf("\n");
#endif
    return DTOF_RET_SUCCESS;
}

static DTOF_RET hal_next_ac_data_config(uint8_t device_id, dtof_uint16_t *next_ac_data_p, dtof_uint16_t length)
{
    DTOF_RET ret;

    if(!next_ac_data_p){
        DTOF_LOG("file: %s, line: %d, pointer is null\n", __FILE__, __LINE__);
        return DTOF_RET_ERROR;
    }

    ret = dtof_reg_burst_write(device_id, DTOF_REG172, next_ac_data_p, length);
    if(ret != DTOF_RET_SUCCESS){
        DTOF_LOG("file: %s, line: %d, reg write fail\n", __FILE__, __LINE__);
        return ret;
    }
    return ret;
}

static DTOF_RET hal_next_dc_data_config(uint8_t device_id, dtof_uint16_t next_dc_data)
{
    DTOF_RET ret;
    ret = dtof_reg_burst_write(device_id, DTOF_REG188, &next_dc_data, 1);
    if(ret != DTOF_RET_SUCCESS){
        DTOF_LOG("file: %s, line: %d, reg write fail\n", __FILE__, __LINE__);
        return ret;
    }
    return ret;
}

DTOF_RET dtof_do_cross_talk_calibration(uint8_t device_id, uint16_t *ct_reg_data)
{
    DTOF_CHECK_PARAM(ct_reg_data, "串扰校准参数为空");

    cross_talk_data_t cross_talk_data;

    // 采样串扰数据
    DTOF_CHECK_RET(dtof_cross_talk_sample_data(device_id, DTOF_CG_CALIB_FRAME_SIZE, cross_talk_data.cross_talk_sample_data),
                    "串扰数据采样失败");

    // 计算串扰校准参数
    DTOF_CHECK_RET(dtof_cross_talk_calibrate_calculate(device_id,
                    cross_talk_data.cross_talk_sample_data,
                    cross_talk_data.cross_talk_bin_data,
                    cross_talk_data.next_ac,
                    &cross_talk_data.next_dc),
                    "串扰校准计算失败");

    // 更新串扰校准寄存器
    DTOF_CHECK_RET(hal_next_ac_data_config(device_id, cross_talk_data.next_ac, DTOF_CT_PARAM_SIZE), "配置next_ac寄存器失败");
    DTOF_CHECK_RET(hal_next_dc_data_config(device_id, cross_talk_data.next_dc), "配置next_dc寄存器失败");

    memcpy(ct_reg_data, cross_talk_data.next_ac, DTOF_CT_PARAM_SIZE * sizeof(uint16_t));
    memcpy(ct_reg_data + DTOF_CT_PARAM_SIZE, &(cross_talk_data.next_dc), sizeof(uint16_t));

#ifdef DTOF_CT_DEBUG
    printf("ct_reg_data:\n");
    for (int i = 0; i < DTOF_CT_PARAM_SIZE + 1; i++)
    {
        printf("%d, ", ct_reg_data[i]);
    }
    printf("\n");
#endif

    return DTOF_RET_SUCCESS;
}
