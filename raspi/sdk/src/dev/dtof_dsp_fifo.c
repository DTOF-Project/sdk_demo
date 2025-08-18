/*
 * dtof_dsp_fifo.c
 *
 *  Created on: 2024年4月30日
 *      Author: liuzihao
 */

#include "inc/dtof_base_type.h"
#include "inc/dev/dtof_dsp_fifo.h"
#include "inc/dev/dtof_hal.h"

void dtof_fifo_decode(dsh_output_fifo_t *fifo_info, dtof_uint16_t mode)
{
    dsh_fifo_info hslinfo;
    dtof_uint16_t index;

    switch (mode)
    {
    case SINGLE_MODE:
    {
        dtof_dsp_fifo_read(JINAN_FIFO_ADDRESS_SINGLE_OFFSET, (dtof_uint16_t *)&hslinfo.peak_info_single, JINAN_FIFO_ADDRESS_SINGLE_SIZE);

        fifo_info->dsh_output_fifo_single.frameid = hslinfo.peak_info_single.frameId;
        fifo_info->dsh_output_fifo_single.mp0 = MERGE_INTEGER_DECIMALS(hslinfo.peak_info_single.roimdqlf0andmmxpf0.pos, hslinfo.peak_info_single.saSgnRO0andsubAccRO0.accpos, hslinfo.peak_info_single.saSgnRO0andsubAccRO0.positve);
        fifo_info->dsh_output_fifo_single.sp0 = MERGE_INTEGER_DECIMALS(hslinfo.peak_info_single.roiSDqlf0andSMXF0.pos, hslinfo.peak_info_single.saSgnR01andsubAccR01.accpos, hslinfo.peak_info_single.saSgnR01andsubAccR01.positve);
        fifo_info->dsh_output_fifo_single.tp0 = MERGE_INTEGER_DECIMALS(hslinfo.peak_info_single.roiTDqlf0andtmxpF0.pos, hslinfo.peak_info_single.saSgnR02andsubAccR02.accpos, hslinfo.peak_info_single.saSgnR02andsubAccR02.positve);
        fifo_info->dsh_output_fifo_single.mp1 = MERGE_INTEGER_DECIMALS(hslinfo.peak_info_single.roiMDqlf1andmmxpF1.pos, hslinfo.peak_info_single.saSgnR10andsubAccR10.accpos, hslinfo.peak_info_single.saSgnR10andsubAccR10.positve);

        // encode hist values
        fifo_info->dsh_output_fifo_single.chanPeakinfo[0].peakshist[0] = hslinfo.peak_info_single.mmxvR0; // mv0
        __asm__("nop");
        fifo_info->dsh_output_fifo_single.chanPeakinfo[0].peakshist[1] = hslinfo.peak_info_single.smxvR0; // sv0
        __asm__("nop");
        fifo_info->dsh_output_fifo_single.chanPeakinfo[0].peakshist[2] = hslinfo.peak_info_single.tmxvR0; // tv0
        __asm__("nop");
        fifo_info->dsh_output_fifo_single.chanPeakinfo[0].flashcount = hslinfo.peak_info_single.hgmFlsCnt1;
        // init flag
        __asm__("nop");
        fifo_info->dsh_output_fifo_single.chanPeakinfo[0].peakstatus = 0;
        __asm__("nop");
        // encode peakspos
        if (!!(hslinfo.peak_info_single.roimdqlf0andmmxpf0.valid & 0x01))
        {
            fifo_info->dsh_output_fifo_single.chanPeakinfo[0].peakspos[0] = hslinfo.peak_info_single.roimdqlf0andmmxpf0.pos;
            fifo_info->dsh_output_fifo_single.chanPeakinfo[0].peakstatus |= DSH_FIFO_MAIN_POSITION;
        }

        if (!!(hslinfo.peak_info_single.roiSDqlf0andSMXF0.valid & 0x01))
        {
            fifo_info->dsh_output_fifo_single.chanPeakinfo[0].peakspos[1] = hslinfo.peak_info_single.roiSDqlf0andSMXF0.pos;
            fifo_info->dsh_output_fifo_single.chanPeakinfo[0].peakstatus |= DSH_FIFO_SECOND_POSITION;
        }

        if (!!(hslinfo.peak_info_single.roiTDqlf0andtmxpF0.valid & 0x01))
        {
            fifo_info->dsh_output_fifo_single.chanPeakinfo[0].peakspos[2] = hslinfo.peak_info_single.roiTDqlf0andtmxpF0.pos;
            fifo_info->dsh_output_fifo_single.chanPeakinfo[0].peakstatus |= DSH_FIFO_THIRD_POSITION;
        }

        // encode acc
        if (!!(hslinfo.peak_info_single.saSgnRO0andsubAccRO0.positve & 0x01))
        {
            fifo_info->dsh_output_fifo_single.chanPeakinfo[0].peakstatus |= DSH_FIFO_MAIN_ACCURACY;
        }
        fifo_info->dsh_output_fifo_single.chanPeakinfo[0].peaksaccpos[0] = hslinfo.peak_info_single.saSgnRO0andsubAccRO0.accpos;

        if (!!(hslinfo.peak_info_single.saSgnR01andsubAccR01.positve & 0x01))
        {
            fifo_info->dsh_output_fifo_single.chanPeakinfo[0].peakstatus |= DSH_FIFO_SECOND_ACCURACY;
        }
        fifo_info->dsh_output_fifo_single.chanPeakinfo[0].peaksaccpos[1] = hslinfo.peak_info_single.saSgnR01andsubAccR01.accpos;

        if (!!(hslinfo.peak_info_single.saSgnR02andsubAccR02.positve & 0x01))
        {
            fifo_info->dsh_output_fifo_single.chanPeakinfo[0].peakstatus |= DSH_FIFO_THIRD_ACCURACY;
        }
        fifo_info->dsh_output_fifo_single.chanPeakinfo[0].peaksaccpos[2] = hslinfo.peak_info_single.saSgnR02andsubAccR02.accpos;

        fifo_info->dsh_output_fifo_single.chanPeakinfo[0].avgHist = hslinfo.peak_info_single.avgQ0 + (hslinfo.peak_info_single.roimdqlf0andmmxpf0.reserved << 16);
        // Standard Deviation
        fifo_info->dsh_output_fifo_single.chanPeakinfo[0].stddHist = hslinfo.peak_info_single.stdQ0;
        __asm__("nop");
        // ref encode
        // encode hist values
        fifo_info->dsh_output_fifo_single.chanPeakinfo[1].peakshist[0] = hslinfo.peak_info_single.mmxvR1; // mv1
        __asm__("nop");
        fifo_info->dsh_output_fifo_single.chanPeakinfo[1].peakshist[1] = hslinfo.peak_info_single.smxvR1;
        __asm__("nop");
        fifo_info->dsh_output_fifo_single.chanPeakinfo[1].peakshist[2] = hslinfo.peak_info_single.tmxvR1;
        __asm__("nop");
        fifo_info->dsh_output_fifo_single.chanPeakinfo[1].flashcount = hslinfo.peak_info_single.hgmFlsCnt1;
        fifo_info->dsh_output_fifo_single.chanPeakinfo[1].peakstatus = 0;
        // encode peakspos
        if (!!(hslinfo.peak_info_single.roiMDqlf1andmmxpF1.valid & 0x01))
        {
            fifo_info->dsh_output_fifo_single.chanPeakinfo[1].peakstatus |= DSH_FIFO_MAIN_POSITION;
        }
        fifo_info->dsh_output_fifo_single.chanPeakinfo[1].peakspos[0] = hslinfo.peak_info_single.roiMDqlf1andmmxpF1.pos;

        if (!!(hslinfo.peak_info_single.roiSDqlf1andSMXPF1.valid & 0x01))
        {
            fifo_info->dsh_output_fifo_single.chanPeakinfo[1].peakstatus |= DSH_FIFO_SECOND_POSITION;
        }
        fifo_info->dsh_output_fifo_single.chanPeakinfo[1].peakspos[1] = hslinfo.peak_info_single.roiSDqlf1andSMXPF1.pos;

        if (!!(hslinfo.peak_info_single.roiTDqlf1andtmxpF1.valid & 0x01))
        {
            fifo_info->dsh_output_fifo_single.chanPeakinfo[1].peakstatus |= DSH_FIFO_THIRD_POSITION;
        }
        fifo_info->dsh_output_fifo_single.chanPeakinfo[1].peakspos[2] = hslinfo.peak_info_single.roiTDqlf1andtmxpF1.pos;
        // encode acc
        if (!!(hslinfo.peak_info_single.saSgnR10andsubAccR10.positve & 0x01))
        {
            fifo_info->dsh_output_fifo_single.chanPeakinfo[1].peakstatus |= DSH_FIFO_MAIN_ACCURACY;
        }
        fifo_info->dsh_output_fifo_single.chanPeakinfo[1].peaksaccpos[0] = hslinfo.peak_info_single.saSgnR10andsubAccR10.accpos;

        if (!!(hslinfo.peak_info_single.saSgnR11andsubAccR11.positve & 0x01))
        {
            fifo_info->dsh_output_fifo_single.chanPeakinfo[1].peakstatus |= DSH_FIFO_SECOND_ACCURACY;
        }
        fifo_info->dsh_output_fifo_single.chanPeakinfo[1].peaksaccpos[1] = hslinfo.peak_info_single.saSgnR11andsubAccR11.accpos;

        if (!!(hslinfo.peak_info_single.saSgnR12andsubAccR12.positve & 0x01))
        {
            fifo_info->dsh_output_fifo_single.chanPeakinfo[1].peakstatus |= DSH_FIFO_THIRD_ACCURACY;
        }
        fifo_info->dsh_output_fifo_single.chanPeakinfo[1].peaksaccpos[2] = hslinfo.peak_info_single.saSgnR12andsubAccR12.accpos;

        fifo_info->dsh_output_fifo_single.chanPeakinfo[1].avgHist = hslinfo.peak_info_single.avgQ1 + (hslinfo.peak_info_single.roiMDqlf1andmmxpF1.reserved << 16);
        // Standard Deviation
        fifo_info->dsh_output_fifo_single.chanPeakinfo[1].stddHist = hslinfo.peak_info_single.stdQ1;
        break;
    }
    case MULTIPLE_MODE:
    {
        dtof_dsp_fifo_read(JINAN_FIFO_ADDRESS_MULTIPLY_OFFSET, (dtof_uint16_t *)&hslinfo.peak_info_multiple, JINAN_FIFO_ADDRESS_ST_MULTIPLY_SIZE);
        for (index = 0; index < DSH_MAX_PEAKINFO_CHAN_MULTIPLE; index++)
        {
            fifo_info->dsh_output_fifo_multiple.chanPeakinfo_multiple[index].peaksvalue = hslinfo.peak_info_multiple.peak_info[index].peak_value;
            fifo_info->dsh_output_fifo_multiple.chanPeakinfo_multiple[index].peakspos = hslinfo.peak_info_multiple.peak_info[index].peak_position.pos;
        }
        // flash_count7-1通道的值在fifo 0x42b-0X431, flash_count0在fifo 0x409, flash_count8在fifo 0x415, 0x409高四位被avgQ0占用, 使用0x415的值
        for (index = 1; index < (DSH_MAX_PEAKINFO_CHAN_MULTIPLE - 1); index++)
        {
            fifo_info->dsh_output_fifo_multiple.flash_count[index] = hslinfo.peak_info_multiple.hgmFlsCnt[DSH_MAX_PEAKINFO_CHAN_MULTIPLE - 2 - index];
        }
        {
            // dtof_uint16_t flscnt_ch0;
            // dtof_uint16_t flscnt_ch8;
            // dtof_uint16_t frame_id;
            dtof_dsp_fifo_read(JINAN_FIFO_ADDRESS_MAIN_FLSCNT_OFFSET, &(fifo_info->dsh_output_fifo_multiple.flash_count[0]), 1);
            dtof_dsp_fifo_read(JINAN_FIFO_ADDRESS_REF_FLSCNT_OFFSET, &(fifo_info->dsh_output_fifo_multiple.flash_count[8]), 1);
            dtof_dsp_fifo_read(JINAN_FIFO_ADDRESS_FRAME_ID_OFFSET, &(fifo_info->dsh_output_fifo_multiple.frame_id), 1);
            // fifo_info->dsh_output_fifo_multiple.frame_id = frame_id;
            // fifo_info->dsh_output_fifo_multiple.flash_count[0] = flscnt_ch0;
            // fifo_info->dsh_output_fifo_multiple.flash_count[8] = flscnt_ch8;
            fifo_info->dsh_output_fifo_multiple.stdR0 = hslinfo.peak_info_multiple.stdR0andXtDcftr.stdR0;
            fifo_info->dsh_output_fifo_multiple.xtDfctR = (hslinfo.peak_info_multiple.stdR0andXtDcftr.xtDcftR << 16) + hslinfo.peak_info_multiple.xtDfctRL;
        }
        break;
    }
    default:
    {
        break;
    }
    }
    return;
}
