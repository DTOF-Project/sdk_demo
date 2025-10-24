/*
 * dtof_dsp_fifo.h
 *
 *  Created on: 2024/8/5
 *      Author: liuzihao
 */

#ifndef _DTOF_DSP_FIFO_H_
#define _DTOF_DSP_FIFO_H_

#ifdef __cplusplus
extern "C" {
#endif

#define SINGLE_MODE   0
#define MULTIPLE_MODE 1

// Jinan RAM  FIFO base
#define JINAN_FIFO_ADDRESS_BASE   0x0400
// Jinan  RAM Fifo size
#define JINAN_FIFO_ADDRESS_SINGLE_SIZE         0x0019
#define JINAN_FIFO_ADDRESS_AMS_MULTIPLY_SIZE   0x0012
#define JINAN_FIFO_ADDRESS_ST_MULTIPLY_SIZE    0x001B
#define JINAN_FIFO_ADDRESS_SINGLE_OFFSET   0
#define JINAN_FIFO_ADDRESS_MULTIPLY_OFFSET (JINAN_FIFO_ADDRESS_SINGLE_OFFSET + JINAN_FIFO_ADDRESS_SINGLE_SIZE)
#define JINAN_FIFO_ADDRESS_FRAME_ID_OFFSET     0x0000
#define JINAN_FIFO_ADDRESS_MAIN_FLSCNT_OFFSET  0x0009
#define JINAN_FIFO_ADDRESS_REF_FLSCNT_OFFSET   0x0015

#define DSH_MAX_PEAKINFO_CHAN 2
#define DSH_MAX_PEAKINFO_CHAN_MULTIPLE 9

#define DSH_FIFO_MAIN_POSITION 0x0001
#define DSH_FIFO_SECOND_POSITION 0x0002
#define DSH_FIFO_THIRD_POSITION 0x0004
// set  mean positive, else negtive
#define DSH_FIFO_MAIN_ACCURACY 0x0010
#define DSH_FIFO_SECOND_ACCURACY 0x0020
#define DSH_FIFO_THIRD_ACCURACY 0x0040

#define MERGE_INTEGER_DECIMALS(x, y, s) ((s) == 1 ? ((x) << 9) - (y) : ((x) << 9) + (y))

typedef enum
{
    dsh_fifo_mainpeak = 0,
    dsh_fifo_secpeak,
    dsh_fifo_thirdpeak,
    dsh_fifo_maxpeak
} dsh_fifoEnum_t;

typedef struct dsh_fifopeakinfo_s
{
    dtof_uint16_t peakstatus;
    dtof_uint16_t flashcount;
    dtof_uint32_t avgHist;
    dtof_uint16_t peakshist[dsh_fifo_maxpeak];
    dtof_uint16_t peakspos[dsh_fifo_maxpeak];
    dtof_uint16_t peaksaccpos[dsh_fifo_maxpeak];
    dtof_uint16_t stddHist;
} dsh_fifopeakinfo_t;

typedef struct dsh_fifopeakinfo_multiple_s
{
    dtof_uint16_t peaksvalue;
    dtof_uint16_t peakspos;
} dsh_fifopeakinfo_multiple_t;

typedef struct
{
    dtof_uint32_t frameid;
    dtof_uint32_t mp0;
    dtof_uint32_t sp0;
    dtof_uint32_t tp0;
    dtof_uint32_t mp1;
    dsh_fifopeakinfo_t chanPeakinfo[DSH_MAX_PEAKINFO_CHAN];
} dsh_output_fifo_single_t;

typedef struct
{
    dtof_uint16_t frame_id;
    dsh_fifopeakinfo_multiple_t chanPeakinfo_multiple[DSH_MAX_PEAKINFO_CHAN_MULTIPLE];
    dtof_uint16_t flash_count[DSH_MAX_PEAKINFO_CHAN_MULTIPLE];
    dtof_uint16_t stdR0;
    dtof_uint32_t xtDfctR;
} dsh_output_fifo_multiple_t;

typedef union
{
    dsh_output_fifo_single_t dsh_output_fifo_single;
    dsh_output_fifo_multiple_t dsh_output_fifo_multiple;
} dsh_output_fifo_t;

#pragma pack(2)

typedef struct dsh_jingan_pos_s
{
    // st的情况下, roimdqlf0andmmxpf0和roiMDqlf1andmmxpF1的6bits保留位为avgQ0的14-19bit
    dtof_uint16_t pos : 9;
    dtof_uint16_t valid : 1;
    dtof_uint16_t reserved : 6;
} dsh_jingan_pos_t;

typedef struct dsh_jingan_acc_s
{
    dtof_uint16_t accpos : 8;
    dtof_uint16_t positve : 1;
    dtof_uint16_t reserved : 7;
} dsh_jingan_acc_t;

typedef struct dsh_jingan_avgh_s
{
    dtof_uint16_t avgR0h : 4;
    dtof_uint16_t avgR1h : 4;
    dtof_uint16_t reserved : 8;
} dsh_jingan_avgh_t;

typedef struct dsh_jingan_pos_multiple_s
{
    dtof_uint16_t pos : 9;
    dtof_uint16_t reserved : 7;
} dsh_jingan_pos_multiple_t;

typedef struct dsh_jingan_stdr0_and_xtdcftr_s
{
    dtof_uint16_t xtDcftR : 5;
    dtof_uint16_t stdR0 : 11;
} dsh_jingan_stdr0_and_xtdcftr_t;

typedef struct dsh_jingan_fifo_multiple_pos_and_val_s
{
    dtof_uint16_t peak_value;
    dsh_jingan_pos_multiple_t peak_position;
} dsh_jingan_fifo_multiple_pos_and_val_t;

typedef struct dsh_jingan_fifo_multiple_s
{
    dsh_jingan_fifo_multiple_pos_and_val_t peak_info[DSH_MAX_PEAKINFO_CHAN_MULTIPLE];
    // hgmFlsCnt[0]和hgmFlsCnt[9]在single fifo中
    dtof_uint16_t hgmFlsCnt[DSH_MAX_PEAKINFO_CHAN_MULTIPLE - 2];
    dtof_uint16_t xtDfctRL;
    dsh_jingan_stdr0_and_xtdcftr_t stdR0andXtDcftr;

} dsh_jingan_fifo_multiple_t;

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
    // TIPS: should multiple 16
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
} dsh_jingan_fifo_single_t;

typedef union
{
    dsh_jingan_fifo_single_t peak_info_single;
    // 25th - 42th
    dsh_jingan_fifo_multiple_t peak_info_multiple;
} dsh_fifo_info;

#pragma pack()

#ifdef __cplusplus
}
#endif

#endif // _DTOF_DSP_FIFO_H_


void dtof_fifo_decode(dsh_output_fifo_t *fifo_info, dtof_uint16_t mode);