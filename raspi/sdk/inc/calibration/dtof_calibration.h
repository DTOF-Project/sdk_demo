#ifndef _DTOF_CALIBRATION_H_
#define _DTOF_CALIBRATION_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "inc/dtof_api.h"
#include "inc/calibration/dtof_calibration_common.h"
#include "inc/calibration/dtof_calibration_reference_spad.h"
#include "inc/calibration/dtof_calibration_cg.h"

#define DTOF_REF_SPAD_MAX 1  // refspad数量取最大

// 片外校准类型定义
typedef enum {
    DTOF_CALIBRATION_REF_SPAD = 0,         // ref spad 校准
    DTOF_CALIBRATION_REF_BINOFFSET,        // binoffset 校准
    DTOF_CALIBRATION_CROSS_TALK_TO_SKY,    // CG 对天校准
    DTOF_CALIBRATION_CROSS_TALK_TO_OBJECT, // CG 对近处物体校准, 目前算法针对50cm距离的物体
    DTOF_CALIBRATION_DOUBLE_POINT_CG,      // 双点校准 CG主峰和远点的两点校准
    DTOF_CALIBRATION_DOUBLE_POINT_FAR,     // 双点校准 远点校准
    DTOF_CALIBRATION_DOUBLE_POINT_NEAR,    // 双点校准 近点校准
    DTOF_CALIBRATION_SINGLE_POINT,         // 单点校准 k值固定, 校准b值
    DTOF_CALIBRATION_CLEAR,   // 清除所有校准参数
    DTOF_CALIBRATION_CONFIG,  // 配置校准参数
    DTOF_CALIBRATION_ALL,     // 做所有校准
    DTOF_CALIBRATION_GET_UUID,// get uuid
    DTOF_CALIBRATION_PERFORMANCE, // 性能测试, snr / fwhm
    DTOF_CALIBRATION_INFO_PRINT, // debug print
} dtof_calibrate_type_t;

#pragma pack(1)

typedef struct {
    dtof_uint16_t spad_mask;
    dtof_spad_info_t spad_info[DTOF_REF_SPAD_MAX_NUM];
} refspad_cal_t;

typedef struct {
    dtof_uint16_t ref_cal_mp1;
    dtof_uint16_t binoffset;
} binoffset_cal_t;

typedef struct {
    dtof_real32_t k;
    dtof_real32_t b;
} kb_data_t;

typedef struct {
    dtof_real32_t snr;
    dtof_uint16_t fwhm;
} performance_cal_t;

#define CAL_PEAK_POS_NUM    2

typedef struct {
    dtof_real32_t mp1; // ref main peak pos
    dtof_real32_t peak_pos[CAL_PEAK_POS_NUM]; // 近点和远点的 peak_pos
} distance_cal_t;

typedef struct {
    dtof_real32_t mp0;
    dtof_real32_t mp1;
    dtof_uint16_t real_distance;
} distance_cal_old_t;

typedef struct {
    dtof_uint32_t refspad_valid : 1;
    dtof_uint32_t crosstalk_valid : 1;
    dtof_uint32_t binwidth_valid : 1;
    dtof_uint32_t offset_valid : 1;
    dtof_uint32_t neardis_valid : 1;
    dtof_uint32_t fardis_valid : 1;
    dtof_uint32_t res : 26;
} valid_mask_t;

#define PARAMETER_VERSION 0xA001
typedef struct {
    dtof_uint16_t version;
    dtof_uint8_t uuid[DTOF_UUID_LENGTH];
    valid_mask_t valid_flag;
    refspad_cal_t rs_data;
    binoffset_cal_t binoffset_data;
    cross_talk_data_t ct_data;
    kb_data_t distance_data;
    distance_cal_old_t near_dis;
    distance_cal_old_t far_dis;
    performance_cal_t performance;
    // timebin_data_t tb_data;
    // sal_uint8_t check_sum;
    // sal_uint8_t res[6];  // 8 bytes对齐
} dtof_calibrate_data_t;

#pragma pack()

dtof_calibrate_data_t* get_calib_param(void);
DTOF_RET dtof_do_calibration(dtof_calibrate_type_t type, dtof_uint16_t actualDis, dtof_calibrate_data_t *calibrate_data_p);

#ifdef __cplusplus
}
#endif
#endif