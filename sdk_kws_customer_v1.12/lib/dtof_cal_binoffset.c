#pragma GCC diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"

#include <stdint.h>
#include "inc/dtof_log.h"
#include "inc/dtof_base_type.h"
#include "inc/dtof_driver.h"
#include "inc/dtof_api.h"
#include "dtof_reg.h"
#include "dtof_hal.h"

// 参考bin配置参数
#define DTOF_REF_BIN_BUFFER_SIZE    64      // bin缓冲区大小
#define DTOF_REF_BIN_TOTAL_LENGTH   256     // 逻辑bin总长度
#define DTOF_REF_BIN_OFFSET_STEP    32      // 偏移步进值
#define DTOF_REF_BIN_SEARCH_MAX     8       // 最大搜索次数
#define DTOF_REF_BIN_FRAME_COUNT    100     // 确认帧数
#define DTOF_REF_RAM_START_ADDR     512     // RAM起始地址
#define DTOF_REF_BIN_OFFSET_SUB     29      // 偏移校准减数

typedef struct {
    uint32_t value;
    uint32_t position;
} dtof_peak_info_t;

typedef struct {
    dtof_uint16_t ref_cal_mp1;
    dtof_uint16_t binoffset;
} binoffset_cal_t;

static DTOF_RET dtof_refbinoffset_set(dtof_uint8_t device_id, uint16_t offset)
{
    DTOF_CHECK_RET(dtof_reg_burst_write(device_id, DTOF_REG226, &offset, 1), "set binoffset failed\n");
    return DTOF_RET_SUCCESS;
}

static void dtof_find_peak_16(const uint16_t *data, uint32_t length, dtof_peak_info_t *peak)
{
    DTOF_CHECK_PTR_VOID(data && peak && length > 0);

    peak->value = data[0];
    peak->position = 0;

    for(uint32_t i = 1; i < length; i++) {
        if(data[i] > peak->value) {
            peak->value = data[i];
            peak->position = i;
        }
    }
}

static void dtof_find_peak_32(const uint32_t *data, uint32_t length, dtof_peak_info_t *peak)
{
    DTOF_CHECK_PTR_VOID(data && peak && length > 0);

    peak->value = data[0];
    peak->position = 0;

    for(uint32_t i = 1; i < length; i++) {
        if(data[i] > peak->value) {
            peak->value = data[i];
            peak->position = i;
        }
    }
}

uint16_t dtof_max_refbin_find(dtof_uint8_t device_id)
{
    dtof_peak_info_t best_peak = {0, 0};

    // 遍历所有可能的offset
    for(uint16_t offset = 0; offset < DTOF_REF_BIN_TOTAL_LENGTH; offset += DTOF_REF_BIN_OFFSET_STEP) {
        // 设置当前offset
        dtof_refbinoffset_set(device_id, offset);

        // 先做单帧测试
        uint16_t test_frame[DTOF_REF_BIN_BUFFER_SIZE];
        dtof_peak_info_t test_peak = {0, 0};
        dtof_calibration_get_frame_data(device_id, DTOF_REF_RAM_START_ADDR, test_frame, DTOF_REF_BIN_BUFFER_SIZE);

        dtof_find_peak_16(test_frame, DTOF_REF_BIN_BUFFER_SIZE, &test_peak);

        // 如果找到明显的峰值, 进行100帧确认
        if(test_peak.value > best_peak.value) {
            best_peak.value = test_peak.value;
            best_peak.position = test_peak.position;
        }

        // 需要优化的地方 可以计算整个窗口的 能量 如果小了 说明 一定是下降趋势 这里先不做优化 Fred Lei
    }

    return best_peak.position;
}

static uint16_t dtof_get_average_peak(dtof_uint8_t device_id)
{
    dtof_peak_info_t peak = {0};
    uint32_t avg_frame[DTOF_REF_BIN_BUFFER_SIZE] = {0};
    uint16_t current_frame[DTOF_REF_BIN_BUFFER_SIZE];

    // 采集多帧数据并平均
    for(uint32_t frame = 0; frame < DTOF_REF_BIN_FRAME_COUNT; frame++) {
        DTOF_CHECK_RET(dtof_calibration_get_frame_data(device_id, DTOF_REF_RAM_START_ADDR, current_frame, DTOF_REF_BIN_BUFFER_SIZE), "获取帧数据失败");

        for(uint32_t i = 0; i < DTOF_REF_BIN_BUFFER_SIZE; i++) {
            avg_frame[i] += current_frame[i];
        }
    }

    // 计算平均值
    for(uint32_t i = 0; i < DTOF_REF_BIN_BUFFER_SIZE; i++) {
        avg_frame[i] /= DTOF_REF_BIN_FRAME_COUNT;
    }

    dtof_find_peak_32(avg_frame, DTOF_REF_BIN_BUFFER_SIZE, &peak);
    return peak.position;
}

DTOF_RET dtof_calibration_refbinoffset_calculate(dtof_uint8_t device_id, dtof_uint16_t* binoffset)
{
    DTOF_CHECK_PARAM(binoffset, "binoffset ptr is null\n");

    binoffset_cal_t binoffset_cal;

    uint16_t best_pos = dtof_max_refbin_find(device_id);

    // 调整峰值位置到中心
    if(best_pos > DTOF_REF_BIN_OFFSET_STEP) {
        best_pos -= DTOF_REF_BIN_OFFSET_STEP/2;
    }

    DTOF_CHECK_RET(dtof_refbinoffset_set(device_id, best_pos), "set best binoffset failed\n");

    // 多帧确认
    uint16_t avg_pos = dtof_get_average_peak(device_id);

    // 计算最终偏移值
    binoffset_cal.ref_cal_mp1 = best_pos + avg_pos;
    binoffset_cal.binoffset = best_pos + avg_pos - DTOF_REF_BIN_OFFSET_SUB;

    DTOF_CHECK_RET(dtof_refbinoffset_set(device_id, binoffset_cal.binoffset), "set final binoffset failed\n");

    *binoffset = binoffset_cal.binoffset;

    return DTOF_RET_SUCCESS;
}
