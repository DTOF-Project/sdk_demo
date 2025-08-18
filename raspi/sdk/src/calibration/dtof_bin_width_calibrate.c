#include <stdint.h>

#include "inc/calibration/dtof_bin_width_calibrate.h"
#include "inc/dev/dtof_hal.h"
#include "inc/dtof_log.h"
#include "inc/dev/dtof_reg.h"
#include "inc/dtof_float.h"
#include "inc/dtof_driver.h"

typedef struct {
    dtof_uint16_t mp0;
    dtof_uint16_t mp1;
} kb_value_t;

#pragma pack(2)
#define DSH_MAX_PEAKINFO_CHAN 2
#define DSH_MAX_PEAKINFO_CHAN_MULTIPLE 9

#define DSH_FIFO_MAIN_POSITION 0x0001
#define DSH_FIFO_SECOND_POSITION 0x0002
#define DSH_FIFO_THIRD_POSITION 0x0004
// set  mean positive, else negtive
#define DSH_FIFO_MAIN_ACCURACY 0x0010
#define DSH_FIFO_SECOND_ACCURACY 0x0020
#define DSH_FIFO_THIRD_ACCURACY 0x0040
typedef struct dsh_jingan_pos_s
{
// st的情况下, roimdqlf0andmmxpf0和roiMDqlf1andmmxpF1的6bits保留位为avgQ0的14-19bit
    dtof_uint16_t pos : 9;
    dtof_uint16_t valid : 1;
    dtof_uint16_t reserved : 6;
} dsh_jingan_pos_t ;

typedef struct dsh_jingan_acc_s
{
    dtof_uint16_t accpos : 8;
    dtof_uint16_t positve : 1;
    dtof_uint16_t reserved : 7;
} dsh_jingan_acc_t ;

typedef struct dsh_jingan_avgh_s
{
    dtof_uint16_t avgR0h : 4;
    dtof_uint16_t avgR1h : 4;
    dtof_uint16_t reserved : 8;
} dsh_jingan_avgh_t ;

typedef struct dsh_jingan_pos_multiple_s
{
    dtof_uint16_t pos : 9;
    dtof_uint16_t reserved : 7;
} dsh_jingan_pos_multiple_t ;

typedef struct dsh_jingan_stdr0_and_xtdcftr_s
{
    dtof_uint16_t xtDcftR : 5;
    dtof_uint16_t stdR0   : 11;
} dsh_jingan_stdr0_and_xtdcftr_t ;

typedef struct dsh_jingan_fifo_multiple_pos_and_val_s
{
    dtof_uint16_t peak_value;
    dsh_jingan_pos_multiple_t peak_position;
} dsh_jingan_fifo_multiple_pos_and_val_t ;

typedef struct dsh_jingan_fifo_multiple_s
{
    dsh_jingan_fifo_multiple_pos_and_val_t peak_info[DSH_MAX_PEAKINFO_CHAN_MULTIPLE];
    // hgmFlsCnt[0]和hgmFlsCnt[9]在single fifo中
    dtof_uint16_t hgmFlsCnt[DSH_MAX_PEAKINFO_CHAN_MULTIPLE - 2];
    dtof_uint16_t xtDfctRL;
    dsh_jingan_stdr0_and_xtdcftr_t stdR0andXtDcftr;

} dsh_jingan_fifo_multiple_t ;


typedef struct dsh_jingan_fifo_inf_s
{
    // 0th 0x400 fmID
    dtof_uint16_t frameId;
    // 1st 0x401 mmxvR0 [22,7]
    // main peak
    dtof_uint16_t mmxvR0;
    // 2st 0x402 smxvR0
    dtof_uint16_t smxvR0;
    // 3st 0x403 tmxvR0
    dtof_uint16_t tmxvR0;
    // 4th 0x404 avgQ0
    // caculate the avg after removing the three peaks
    dtof_uint16_t avgQ0;
    // caculate the avg after removing the three peaks  square ()
    // 5th 0x405  stdQ0
    dtof_uint16_t stdQ0;
    // the location of the bin about the main peak
    //  roimdqlf : valid / invalid
    // 6th  0x406 roimdqlf0andmmxpf0
    dsh_jingan_pos_t roimdqlf0andmmxpf0;
    // 7th  0x407 roiSDqlf0 SMXPF0
    dsh_jingan_pos_t roiSDqlf0andSMXF0;
    // 8th  0x408 roiTDqlf0 tmxpF0
    dsh_jingan_pos_t roiTDqlf0andtmxpF0;
    // 9th  0x409 hgmFlsCntR[0] [4+:16]
    // TIPS： should multiple 16
    dtof_uint16_t hgmFlsCnt0;
    // 10th  0x413 saSgnR0[0],subAccRo[0][7:0]
    dsh_jingan_acc_t saSgnRO0andsubAccRO0;
    // 11th  0x414 saSgnR0[1],subAccRo[1][7:0]
    dsh_jingan_acc_t saSgnR01andsubAccR01;
    // 12th  0x415 saSgnR0[2],subAccR0[2][7:0]
    dsh_jingan_acc_t saSgnR02andsubAccR02;
    // 13th -x40a mmxvR1[22:7]
    dtof_uint16_t mmxvR1;
    // 14th 0x40b  smxvR1[22:7]
    dtof_uint16_t smxvR1;
    // 15th 0x40c  tmxvR1[22:7]
    dtof_uint16_t tmxvR1;
    // 16th 0x40d  avgQ1[15:1]
    dtof_uint16_t avgQ1;
    // 17th 0x40e  stdQ1[15:1]
    dtof_uint16_t stdQ1;
    // 18th 0x40f  roiMDqlf1, mmxpF1[8:0]
    dsh_jingan_pos_t roiMDqlf1andmmxpF1;
    // 19th 0x410  roiSDqlf1 SMXPF1
    dsh_jingan_pos_t roiSDqlf1andSMXPF1;
    // 20th  0x411 roiTDqlf1 tmxpF1
    dsh_jingan_pos_t roiTDqlf1andtmxpF1;
    // 21th  0x412 hgmFlsCntR[1]
    dtof_uint16_t hgmFlsCnt1;
    // 22th  0x416 saSgnR1[0],subAccR1[0][7:0]
    dsh_jingan_acc_t saSgnR10andsubAccR10;
    // 23th  0x417 saSgnR1[1],subAccR1[1][7:0]
    dsh_jingan_acc_t saSgnR11andsubAccR11;
    // 24th  0x418 saSgnR1[2],subAccR1[2][7:0]
    dsh_jingan_acc_t saSgnR12andsubAccR12;
} dsh_jingan_fifo_single_t ;

typedef union {
    dsh_jingan_fifo_single_t peak_info_single;
    // 25th - 42th
    dsh_jingan_fifo_multiple_t peak_info_multiple;
}dsh_fifo_info;

#pragma pack()

// #define MERGE_INTEGER_DECIMALS(x, y, s) ((s) == 1 ? ((x) << 9) - (y) : ((x) << 9) + (y))

static dtof_real32_t MERGE_INTEGER_DECIMALS(dtof_uint16_t x, dtof_uint16_t y, dtof_uint16_t s){
    if(s==1){
        return dtof_sub(dtof_from_int(x),dtof_div(dtof_from_int(y),dtof_from_float(512.0f)));
    }else{
        return dtof_add(dtof_from_int(x),dtof_div(dtof_from_int(y),dtof_from_float(512.0f)));
    }
}

/**
 * @brief Get the data from fifo.
 *
 * @param[in,out] kb_info_p The pointer of the @ref kb_value_t structure which stores the data from fifo.
 *
 * @return The function returns the error code. If the function runs successfully, it returns @ref DTOF_RET_SUCCESS.
 *         Otherwise it returns the corresponding error code.
 */
static DTOF_RET dtof_get_dis_cal_data(distance_cal_t * distance_info_p) {
    dsh_fifo_info hslinfo;
    dtof_dsp_fifo_read(0, (dtof_uint16_t *)&hslinfo.peak_info_single, DTOF_FIFO_ADDR_SINGLE_SIZE);

    distance_info_p->peak_pos[DTOF_FAR_PEAK_POS_INDEX] = MERGE_INTEGER_DECIMALS(hslinfo.peak_info_single.roimdqlf0andmmxpf0.pos, hslinfo.peak_info_single.saSgnRO0andsubAccRO0.accpos, hslinfo.peak_info_single.saSgnRO0andsubAccRO0.positve);
    distance_info_p->peak_pos[DTOF_NEAR_PEAK_POS_INDEX] = MERGE_INTEGER_DECIMALS(hslinfo.peak_info_single.roiSDqlf0andSMXF0.pos, hslinfo.peak_info_single.saSgnR01andsubAccR01.accpos, hslinfo.peak_info_single.saSgnR01andsubAccR01.positve);
    distance_info_p->mp1 = MERGE_INTEGER_DECIMALS(hslinfo.peak_info_single.roiMDqlf1andmmxpF1.pos, hslinfo.peak_info_single.saSgnR10andsubAccR10.accpos, hslinfo.peak_info_single.saSgnR10andsubAccR10.positve);

    // debug code
    DTOF_LOG("fifo mp0 = %d, sp0 = %d, mp1 = %d\n", hslinfo.peak_info_single.roimdqlf0andmmxpf0.pos, hslinfo.peak_info_single.roiSDqlf0andSMXF0.pos, hslinfo.peak_info_single.roiMDqlf1andmmxpF1.pos);
    DTOF_LOG("precision mp0 = %f, sp0 = %f, mp1 = %f\n", distance_info_p->peak_pos[DTOF_FAR_PEAK_POS_INDEX], distance_info_p->peak_pos[DTOF_NEAR_PEAK_POS_INDEX], distance_info_p->mp1);
    return DTOF_RET_SUCCESS;
}

static DTOF_RET dtof_get_dis_cal_data_double_point(distance_cal_old_t *distance_info_p) {
    dsh_fifo_info hslinfo;
    dtof_dsp_fifo_read(0, (dtof_uint16_t *)&hslinfo.peak_info_single, DTOF_FIFO_ADDR_SINGLE_SIZE);

    distance_info_p->mp0 = MERGE_INTEGER_DECIMALS(hslinfo.peak_info_single.roimdqlf0andmmxpf0.pos, hslinfo.peak_info_single.saSgnRO0andsubAccRO0.accpos, hslinfo.peak_info_single.saSgnRO0andsubAccRO0.positve);
    distance_info_p->mp1 = MERGE_INTEGER_DECIMALS(hslinfo.peak_info_single.roiMDqlf1andmmxpF1.pos, hslinfo.peak_info_single.saSgnR10andsubAccR10.accpos, hslinfo.peak_info_single.saSgnR10andsubAccR10.positve);

    DTOF_LOG("fifo mp0 = %d, mp1 = %d, main_maxfls = 0x%4x, ref_maxfls = 0x%4x\n", hslinfo.peak_info_single.roimdqlf0andmmxpf0.pos, hslinfo.peak_info_single.roiMDqlf1andmmxpF1.pos, hslinfo.peak_info_single.hgmFlsCnt0, hslinfo.peak_info_single.hgmFlsCnt1);
    DTOF_LOG("precision mp0 = %f, mp1 = %f\n", distance_info_p->mp0, distance_info_p->mp1);

    return DTOF_RET_SUCCESS;
}


/**
 * @brief This function is used to do single point calibration.
 *
 * @param[in] deviceID   The device ID of the DTOF sensor.
 * @param[in] actualDis  The actual distance of the object.
 * @param[out] pData     The pointer of the @ref distance_cal_t structure which stores the calibration data.
 *
 * @return The function returns the error code. If the function runs successfully, it returns @ref DTOF_RET_OK.
 *         Otherwise it returns the corresponding error code.
 */

#define MAX_LINE_CAL_NUM 30
#define CALIBRATION_MAIN_DATA_PFUNC_MIN_SIZE (19)  // 使用pfunc超精度接口时main缓存最小长度
#define CALIBRATION_REF_DATA_PFUNC_MIN_SIZE (26)  // 使用pfunc超精度接口时ref缓存最小长度

// 原来是#define MP1_VALID_MIN_VALUE    (10), 对吗
#define SP0_VALID_MIN_VALUE    (10)
#define SP0_VALID_MAX_VALUE    (30)

static DTOF_RET dtof_specfic_distance_calirbration(distance_cal_t *data_p) {
    DTOF_RET ret;

    dtof_real32_t avg_mp0 = 0;
    dtof_real32_t avg_sp0 = 0;
    dtof_real32_t avg_mp1 = 0;

    dtof_uint16_t count = 0;
    dtof_uint16_t index = 0;
    distance_cal_t calc_data;
    // dtof_uint16_t fifo_data[DTOF_FIFO_ADDR_SINGLE_SIZE];

    if (!data_p) {
        return(DTOF_RET_ERROR);
    }

    for (index = 0; index < MAX_LINE_CAL_NUM; index++) {
        // run once 触发
        ret = hal_dtof_prepare_one_frame();
        if (DTOF_RET_SUCCESS != ret) {
            DTOF_LOG("prepare one frame fail\n");
            return(DTOF_RET_ERROR);
        }

        ret = dtof_get_dis_cal_data(&calc_data);
        if (DTOF_RET_SUCCESS != ret) {
            DTOF_LOG("get fifo data fail\n");
            return(DTOF_RET_ERROR);
        }

        // cg的峰在10-30之间 (5120 ~ 15360)
        if ((calc_data.peak_pos[DTOF_NEAR_PEAK_POS_INDEX] < SP0_VALID_MIN_VALUE) || (calc_data.peak_pos[DTOF_NEAR_PEAK_POS_INDEX] > SP0_VALID_MAX_VALUE)) {
            DTOF_LOG("invalid cg peak\n");
            continue;
        }

        avg_mp0 += calc_data.peak_pos[DTOF_FAR_PEAK_POS_INDEX];
        avg_sp0 += calc_data.peak_pos[DTOF_NEAR_PEAK_POS_INDEX];
        avg_mp1 += calc_data.mp1;
        ++count;
    }
    if (count < 5) {
        DTOF_LOG("vaild count < 5, calibration fail\n");
        return(DTOF_RET_ERROR);
    }

    data_p->peak_pos[DTOF_FAR_PEAK_POS_INDEX] = avg_mp0 / count;
    data_p->peak_pos[DTOF_NEAR_PEAK_POS_INDEX] = avg_sp0 / count;
    data_p->mp1 = avg_mp1 / count;

    DTOF_LOG("avg mp0 = %f, sp0 = %f, mp1 = %f\n", data_p->peak_pos[DTOF_FAR_PEAK_POS_INDEX], data_p->peak_pos[DTOF_NEAR_PEAK_POS_INDEX], data_p->mp1);

    return(DTOF_RET_SUCCESS);
}

DTOF_RET dtof_do_distance_calibration(dtof_uint16_t far_distance, kb_data_t *kb_data_p) {
#define DIS_CAL_PCGIHEAD0 0
#define DIS_CAL_PCGIFPOK0 40
#define DIS_CAL_MPDL 6
    DTOF_RET ret;
    dtof_uint16_t reg112;
    // dtof_uint16_t reg114;
    // dtof_uint16_t reg117;
    dtof_uint16_t pcgihead0_backup;
    // dtof_uint16_t pcgifpok0;
    // dtof_uint16_t mpdl;
    dtof_addressREG112_t *reg112_p = (dtof_addressREG112_t*)&reg112;
    // dtof_addressREG117_t *reg117_p = (dtof_addressREG117_t*)&reg117;
    distance_cal_t distance_cal_data;

    // disable cg
    ret = hal_cg_config(DTOF_FALSE);
    if(ret != DTOF_RET_SUCCESS) {
        DTOF_LOG("file: %s, line: %d, cg disable fail\n", __FILE__, __LINE__);
        return ret;
    }

    // save config
    ret = dtof_reg_burst_read(device_id, DTOF_REG112, &reg112, 1);
    if(ret != DTOF_RET_SUCCESS) {
        DTOF_LOG("file: %s, line: %d, reg read fail\n", __FILE__, __LINE__);
        return ret;
    }
    // ret = dtof_reg_burst_read(device_id, DTOF_REG114, &reg114, 1);
    // if(ret != DTOF_RET_SUCCESS) {
    //     DTOF_LOG("file: %s, line: %d, reg read fail\n", __FILE__, __LINE__);
    //     return ret;
    // }
    // ret = dtof_reg_burst_read(device_id, DTOF_REG117, &reg117, 1);
    // if(ret != DTOF_RET_SUCCESS) {
    //     DTOF_LOG("file: %s, line: %d, reg read fail\n", __FILE__, __LINE__);
    //     return ret;
    // }
    pcgihead0_backup = reg112_p->pcgiHead0;
    // pcgifpok0 = reg114;
    // mpdl = reg117_p->mpdl;

    // change config
    reg112_p->pcgiHead0 = DIS_CAL_PCGIHEAD0;
    // reg114 = DIS_CAL_PCGIFPOK0;
    // reg117_p->mpdl = DIS_CAL_MPDL;
    ret = dtof_reg_burst_write(device_id, DTOF_REG112, &reg112, 1);
    if(ret != DTOF_RET_SUCCESS) {
        DTOF_LOG("file: %s, line: %d, reg write fail\n", __FILE__, __LINE__);
        return ret;
    }
    // ret = dtof_reg_burst_write(device_id, DTOF_REG114, &reg114, 1);
    // if(ret != DTOF_RET_SUCCESS) {
    //     DTOF_LOG("file: %s, line: %d, reg write fail\n", __FILE__, __LINE__);
    //     return ret;
    // }
    // ret = dtof_reg_burst_write(device_id, DTOF_REG117, &reg117, 1);
    // if(ret != DTOF_RET_SUCCESS) {
    //     DTOF_LOG("file: %s, line: %d, reg write fail\n", __FILE__, __LINE__);
    //     return ret;
    // }

    // data sampling
    ret = dtof_specfic_distance_calirbration(&distance_cal_data);
    if(ret != DTOF_RET_SUCCESS) {
        DTOF_LOG("file: %s, line: %d, dtof_specfic_distance_calirbration fail\n", __FILE__, __LINE__);
        return ret;
    }

    kb_data_p->k = (far_distance) / (distance_cal_data.peak_pos[DTOF_FAR_PEAK_POS_INDEX] - distance_cal_data.peak_pos[DTOF_NEAR_PEAK_POS_INDEX]);
    kb_data_p->b = (far_distance) - (kb_data_p->k * (distance_cal_data.peak_pos[DTOF_FAR_PEAK_POS_INDEX] - distance_cal_data.mp1));

    // recover config
    reg112_p->pcgiHead0 = pcgihead0_backup;
    // reg114 = pcgifpok0;
    // reg117_p->mpdl = mpdl;
    ret = dtof_reg_burst_write(device_id, DTOF_REG112, &reg112, 1);
    if(ret != DTOF_RET_SUCCESS) {
        DTOF_LOG("file: %s, line: %d, reg write fail\n", __FILE__, __LINE__);
        return ret;
    }
    // ret = dtof_reg_burst_write(device_id, DTOF_REG114, &reg114, 1);
    // if(ret != DTOF_RET_SUCCESS) {
    //     DTOF_LOG("file: %s, line: %d, reg write fail\n", __FILE__, __LINE__);
    //     return ret;
    // }
    // ret = dtof_reg_burst_write(device_id, DTOF_REG117, &reg117, 1);
    // if(ret != DTOF_RET_SUCCESS) {
    //     DTOF_LOG("file: %s, line: %d, reg write fail\n", __FILE__, __LINE__);
    //     return ret;
    // }

    // enable cg
    ret = hal_cg_config(DTOF_TRUE);
    if(ret != DTOF_RET_SUCCESS) {
        DTOF_LOG("file: %s, line: %d, cg enable fail\n", __FILE__, __LINE__);
        return ret;
    }

    return ret;
}

static DTOF_RET dtof_specfic_distance_calirbration_double_point(distance_cal_old_t *distance_cal_data_p) {
    DTOF_RET ret;

    dtof_real32_t avg_mp0 = 0;
    dtof_real32_t avg_mp1 = 0;

    dtof_uint16_t count = 0;
    dtof_uint16_t index = 0;
    distance_cal_old_t calc_data;
    // dtof_uint16_t fifo_data[DTOF_FIFO_ADDR_SINGLE_SIZE];

    if (!distance_cal_data_p) {
        return(DTOF_RET_ERROR);
    }

    for (index = 0; index < MAX_LINE_CAL_NUM; index++) {
        // run once 触发
        ret = hal_dtof_prepare_one_frame();
        if (DTOF_RET_SUCCESS != ret) {
            DTOF_LOG("prepare one frame fail\n");
            return(DTOF_RET_ERROR);
        }

        ret = dtof_get_dis_cal_data_double_point(&calc_data);
        if (DTOF_RET_SUCCESS != ret) {
            DTOF_LOG("get fifo data fail\n");
            return(DTOF_RET_ERROR);
        }

        // TODO:@liuzihao 判断mp1....

        avg_mp0 += calc_data.mp0;
        avg_mp1 += calc_data.mp1;
        ++count;
    }
    if (count < 5) {
        DTOF_LOG("vaild count < 5, calibration fail\n");
        return(DTOF_RET_ERROR);
    }

    distance_cal_data_p->mp0 = avg_mp0 / count;
    distance_cal_data_p->mp1 = avg_mp1 / count;

    DTOF_LOG("avg mp0 = %f, mp1 = %f\n", distance_cal_data_p->mp0, distance_cal_data_p->mp1);

    return(DTOF_RET_SUCCESS);
}

DTOF_RET dtof_do_distance_calibration_old(dtof_uint16_t distance, dtof_int32_t cal_type, dtof_calibrate_data_t *calibrate_data_p) {
    DTOF_RET ret;
    distance_cal_old_t *distance_cal_data_p;

    if (!calibrate_data_p) {
        DTOF_LOG("calibrate_data_p is null\n");
        DTOF_CHECK_RET(DTOF_RET_INVALID_PARAM,  "calibrate_data_p is null");
    }

    // 区分近点和远点
    if(cal_type == DTOF_CALIBRATION_DOUBLE_POINT_FAR){
        distance_cal_data_p = &(calibrate_data_p->far_dis);
    }
    else if(cal_type == DTOF_CALIBRATION_DOUBLE_POINT_NEAR){
        distance_cal_data_p = &(calibrate_data_p->near_dis);
    }
    else{
        DTOF_LOG("error calibrate type\n");
        DTOF_CHECK_RET(DTOF_RET_INVALID_PARAM,  "error calibrate type");
    }

    // 保存真实距离
    distance_cal_data_p->real_distance = distance;

    // data sampling
    ret = dtof_specfic_distance_calirbration_double_point(distance_cal_data_p);
    if(ret != DTOF_RET_SUCCESS) {
        DTOF_LOG("file: %s, line: %d, dtof_specfic_distance_calirbration fail\n", __FILE__, __LINE__);
        return ret;
    }

    if(cal_type == DTOF_CALIBRATION_DOUBLE_POINT_FAR){
        calibrate_data_p->valid_flag.fardis_valid = 1;
    }
    else{
        calibrate_data_p->valid_flag.neardis_valid = 1;
    }

    if(calibrate_data_p->valid_flag.fardis_valid && calibrate_data_p->valid_flag.neardis_valid){
        calibrate_data_p->distance_data.k = (calibrate_data_p->far_dis.real_distance - calibrate_data_p->near_dis.real_distance) / (calibrate_data_p->far_dis.mp0 - calibrate_data_p->near_dis.mp0);
        calibrate_data_p->distance_data.b = calibrate_data_p->far_dis.real_distance - (calibrate_data_p->distance_data.k * (calibrate_data_p->far_dis.mp0 - calibrate_data_p->far_dis.mp1));
        // calibrate_data_p->distance_data.b = (far_distance << 9) - (kb_data_p->k * (distance_cal_data.peak_pos[DTOF_FAR_PEAK_POS_INDEX] - distance_cal_data.mp1));
        DTOF_LOG("distance calibration k: %f, b: %f\n", calibrate_data_p->distance_data.k, calibrate_data_p->distance_data.b);
    }

    return ret;
}


DTOF_RET dtof_do_distance_calibration_b(dtof_uint16_t distance, dtof_int32_t cal_type, dtof_calibrate_data_t *calibrate_data_p) {
    DTOF_RET ret;
    distance_cal_old_t *distance_cal_data_p = &calibrate_data_p->far_dis;
    dtof_real32_t mp0;

    if (!calibrate_data_p) {
        DTOF_LOG("calibrate_data_p is null\n");
        DTOF_CHECK_RET(DTOF_RET_INVALID_PARAM,  "calibrate_data_p is null");
    }

    // 保存真实距离
    distance_cal_data_p->real_distance = distance;

    // data sampling
    ret = dtof_specfic_distance_calirbration_double_point(distance_cal_data_p);
    if(ret != DTOF_RET_SUCCESS) {
        DTOF_LOG("file: %s, line: %d, dtof_specfic_distance_calirbration fail\n", __FILE__, __LINE__);
        return ret;
    }

    calibrate_data_p->distance_data.k = 18.71f;
    calibrate_data_p->distance_data.b = calibrate_data_p->far_dis.real_distance - (calibrate_data_p->distance_data.k * (calibrate_data_p->far_dis.mp0 - calibrate_data_p->far_dis.mp1));

    DTOF_LOG("distance calibration k: %f, b: %f\n", calibrate_data_p->distance_data.k, calibrate_data_p->distance_data.b);

    return ret;
}
