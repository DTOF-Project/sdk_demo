/**
 * @file dtof_calibration_common.h
 * @author jiao.xu
 * @brief about calibration common
 * @version 1.0
 * @date 2022-08-11
 *
 * @copyright Copyright (c) 2022
 *
 */
#include "inc/calibration/dtof_calibration_common.h"
#include "sdk/inc/dtof_common.h"
#include "inc/dtof_log.h"
#include "inc/dev/dtof_hal.h"

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

DTOF_RET dtof_find_max_uint16(const uint16_t *buffer, uint16_t size,
                                uint16_t *maxIndex) {
    uint16_t i;
    uint16_t currMaxIndex;
    int16_t maxValue;

    if (!buffer || (0 == size) || !maxIndex) {
        DTOF_LOG("paramter error!\n");
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

DTOF_RET dtof_calc_mean_int16(const int16_t *data, int16_t size, int16_t *mean) {
    int32_t sum = 0;
    int16_t i;
    if (!data || (0 == size) || !mean) {
        DTOF_LOG("paramter error!\n");
        return DTOF_RET_ERROR;
    }
    for (i = 0; i < size; i++) {
        sum += data[i];
        // printf("%d,",data[i]);
    }
    // printf("sum=%d size=%d\n",sum, size);
    *mean = (int16_t)(sum / size);
    return DTOF_RET_SUCCESS;
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

int dtof_calc_fwhm_uint16(const uint16_t *data, uint32_t length, uint32_t peak_index) {
    if ((peak_index >= length) || (peak_index == 0)) {
        DTOF_LOG("peak index out of range\n");
        return 0;
    }

    int peak_value = (data[peak_index] + data[peak_index + 1] + data[peak_index - 1]) / 3;
    int half_value = peak_value / 2;

    // 向左找
    int left_index = peak_index;
    while (left_index > 0 && data[left_index] > half_value) {
        left_index--;
    }

    // 向右找
    int right_index = peak_index;
    while (right_index < length - 1 && data[right_index] > half_value) {
        right_index++;
    }

    int fwhm = right_index - left_index;
    return fwhm;
}



DTOF_RET dtof_calibration_get_frame_data(uint16_t offset, uint16_t *out_buf, uint16_t len)
{
    DTOF_RET ret;
    if (!out_buf)
    {
        DTOF_LOG("param error\r\n");
        return DTOF_RET_ERROR;
    }

    ret = hal_dtof_prepare_one_frame();
    DTOF_CHECK_RET(ret, "prepare one frame fail\n");

    ret = dtof_histgram_io_read(offset, out_buf, len);
    DTOF_CHECK_RET(ret, "read ram fail\n");

    return ret;
}