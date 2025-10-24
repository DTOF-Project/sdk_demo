#include <stdint.h>
#include "inc/dtof_base_type.h"
#include "inc/dev/dtof_hal.h"
#include "inc/dtof_log.h"
#include "inc/dev/dtof_reg.h"
#include "inc/dev/dtof_dsp_fifo.h"
#include "inc/dtof_float.h"
#include "inc/dtof_driver.h"
#include "inc/calibration/dtof_calibration.h"

// b值校准后做性能验证, 输出snr
DTOF_RET dtof_performance_verify(performance_cal_t* performance_cal_p)
{
#define DTOF_SNR_RATE (2.0f)
#define DTOF_FWHM_TEST_RAM_SIZE (512)
    DTOF_RET ret = DTOF_SUCCESS;
    dsh_fifo_info hslinfo;
    dtof_real32_t main_noise;
    // dtof_real32_t snr;
    dtof_uint16_t peak_index;
    // dtof_uint16_t fwhm;
    dtof_uint16_t hist_ram[DTOF_FWHM_TEST_RAM_SIZE];

    DTOF_CHECK_RET(dtof_calibration_get_frame_data(0, hist_ram, DTOF_FWHM_TEST_RAM_SIZE), "dtof get frame data fail\n");

    DTOF_CHECK_RET(dtof_dsp_fifo_read(0, (dtof_uint16_t *)&hslinfo.peak_info_single, JINAN_FIFO_ADDRESS_SINGLE_SIZE), "dsp fifo read fail\n");

    // 计算snr
    main_noise = (hslinfo.peak_info_single.avgQ0 + ((hslinfo.peak_info_single.roimdqlf0andmmxpf0.reserved & 0x3c) << 14)) / 16.0f;
    performance_cal_p->snr = dtof_div(dtof_sub(hslinfo.peak_info_single.mmxvR0 / DTOF_SNR_RATE, main_noise / DTOF_SNR_RATE),
        dtof_sqrt(main_noise / DTOF_SNR_RATE));

    // 计算FWHM
    dtof_find_max_uint16(hist_ram, DTOF_FWHM_TEST_RAM_SIZE, &peak_index);
    performance_cal_p->fwhm = dtof_calc_fwhm_uint16(hist_ram, DTOF_FWHM_TEST_RAM_SIZE, peak_index);

    printf("main_noise = : %.6f\n", main_noise);
    printf("peak_cnt = : %d\n", hist_ram[peak_index]);
    printf("snr = : %.6f\n", performance_cal_p->snr);
    printf("fwhm = : %d\n", performance_cal_p->fwhm);

    return ret;
}

// dtof_performance_verify();