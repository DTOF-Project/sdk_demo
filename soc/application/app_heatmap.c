#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "inc/dtof_base_type.h"
#include "inc/dtof_driver.h"
#include "inc/dtof_api.h"
#include "inc/dtof_log.h"
#include "inc/dev/dtof_hal.h"
#include "inc/dev/dtof_dev_api.h"
#include "inc/dev/dtof_dsp_fifo.h"


void app_heatmap_output(void)
{
#define DTOF_MAIN_SPAD_REG_ADDR 204
#define DTOF_MAIN_SPAD_NUM_ONE_REG 16
#define DTOF_MAIN_SPAD_REG_NUM 4
#define DTOF_REG_NUM_ALL 255

    dtof_uint16_t spad_mask_value = 0;
    dtof_uint16_t peak_cnt[DTOF_MAIN_SPAD_NUM_ONE_REG * DTOF_MAIN_SPAD_REG_NUM];
    dtof_uint16_t nflash[DTOF_MAIN_SPAD_NUM_ONE_REG * DTOF_MAIN_SPAD_REG_NUM];
    dtof_real32_t main_noise[DTOF_MAIN_SPAD_NUM_ONE_REG * DTOF_MAIN_SPAD_REG_NUM];
    dtof_uint16_t dsp_fifo[DTOF_SINGLE_FIFO_LEN];
    dtof_uint16_t reg_all[DTOF_REG_NUM_ALL];

    DTOF_CHECK_WARN(dtof_start_ft_calibrate(), "ft start failed\n");
    DTOF_CHECK_WARN(dtof_set_mcu_status_ram(DTOF_MCU_STATE_SLEEP_DIRECT), "mcu sleep failed\n");

    for(int i = 0; i < DTOF_MAIN_SPAD_REG_NUM; i++)
    {
        DTOF_CHECK_WARN(dtof_reg_burst_write(DTOF_MAIN_SPAD_REG_ADDR + i, &spad_mask_value, 1), "reg write failed\n");
    }
    DTOF_CHECK_WARN(hal_spad_mskreq(), "spad mask request failed\n");

    for(int spad_reg_index = 0; spad_reg_index < DTOF_MAIN_SPAD_REG_NUM; spad_reg_index++) {
        for(int spad_index = 0; spad_index < DTOF_MAIN_SPAD_NUM_ONE_REG; spad_index++) {
            DTOF_CHECK_WARN(hal_spad_mask_config(DTOF_MAIN_SPAD_REG_ADDR + spad_reg_index, 1 << spad_index), "spad mask config failed\n");
            DTOF_CHECK_WARN(hal_dtof_prepare_one_frame(), "prepare one frame failed\n");
            DTOF_CHECK_WARN(dtof_dsp_fifo_read(0, dsp_fifo, DTOF_SINGLE_FIFO_LEN), "dsp fifo read failed\n");
            DTOF_CHECK_WARN(dtof_reg_burst_read(0, reg_all, DTOF_REG_NUM_ALL), "reg read failed\n");

            // save data
            peak_cnt[spad_reg_index * DTOF_MAIN_SPAD_NUM_ONE_REG + spad_index] = dsp_fifo[1] / 2;
            nflash[spad_reg_index * DTOF_MAIN_SPAD_NUM_ONE_REG + spad_index] = dsp_fifo[9];
            main_noise[spad_reg_index * DTOF_MAIN_SPAD_NUM_ONE_REG + spad_index] = (dsp_fifo[4] + ((uint32_t)(dsp_fifo[6] & 0xf000) << 4)) / 32.0f;

            // debug print
            for(int i = 0; i < DTOF_SINGLE_FIFO_LEN; i++) {
                printf("%d, ", dsp_fifo[i]);
            }
            printf("\n");
            for(int i = 0; i < DTOF_REG_NUM_ALL; i++) {
                printf("%d, ", reg_all[i]);
            }
            printf("\n");
        }
        DTOF_CHECK_WARN(hal_spad_mask_config(DTOF_MAIN_SPAD_REG_ADDR + spad_reg_index, spad_mask_value), "spad mask config failed\n");
    }

    // print result
    for(int i = 0; i < DTOF_MAIN_SPAD_NUM_ONE_REG * DTOF_MAIN_SPAD_REG_NUM; i++) {
        printf("%d, ", peak_cnt[i]);
    }
    printf("\n");

    for(int i = 0; i < DTOF_MAIN_SPAD_NUM_ONE_REG * DTOF_MAIN_SPAD_REG_NUM; i++) {
        printf("%d, ", nflash[i]);
    }
    printf("\n");

    for(int i = 0; i < DTOF_MAIN_SPAD_NUM_ONE_REG * DTOF_MAIN_SPAD_REG_NUM; i++) {
        printf("%.6f, ", main_noise[i]);
    }
    printf("\n");

    DTOF_CHECK_WARN(dtof_set_mcu_status_ram(DTOF_MCU_STATE_WAKEUP), "MCU wakeup failed");
    return;
}

