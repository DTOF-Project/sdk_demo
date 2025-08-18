/**
 * @file dtof_calibration_timestamp.c
 * @author jiao.xu
 * @brief 芯片时间戳校准相关功能
 * @version 1.0
 * @date 2022-08-11
 *
 * @copyright Copyright (c) 2022
 */

#include "inc/calibration/dtof_calibration_timestamp.h"
#include "inc/calibration/dtof_calibration_common.h"
#include "inc/dtof_float.h"
#include "inc/dtof_libc.h"
#include "inc/dtof_log.h"

// 时间戳校准参数定义
#define DTOF_TS_CALIB_FREQ           1000    // 校准频率
#define DTOF_TS_CALIB_THRESHOLD      2       // 校准阈值
#define DTOF_TS_TIMER_PERIOD_US      1       // 定时器周期(us)
#define DTOF_TS_CALIB_K_MULTIPLE     1000    // K值倍数
#define DTOF_TS_VCSEL_CYCLE_DEFAULT  64000   // 默认VCSEL周期

// 错误处理宏
#define DTOF_CHECK_PARAM(cond, msg) do { \
    if (!(cond)) { \
        DTOF_LOG("%s\n", msg); \
        return DTOF_RET_ERROR; \
    } \
} while(0)


typedef struct {
    uint32_t      time_sum;          // 时间累加值(us)
    uint32_t      flashn_sum;        // 闪光次数累加值
    uint32_t      frame_time_start;  // 帧起始时间
    uint32_t      frame_time_end;    // 帧结束时间
    uint32_t      timer_period;      // 定时器周期(us)
    uint16_t      vcsel_trig_cycle;  // VCSEL触发周期(us)
    dtof_bool_t   run_flag;          // 校准运行标志
    dtof_real32_t ts_calib_k;        // 校准K值
    dtof_real32_t ts_calib_b;        // 校准B值
    dtof_real32_t last_calib_k;      // 上次校准K值
} dtof_timestamp_calib_t;

#ifdef DTOF_MODULE_TIMESTAMP_CALIB

static dtof_timestamp_calib_t dtof_timestamp_calib;

static dtof_timestamp_calib_t *get_timestamp(void)
{
    return &dtof_timestamp_calib;
}

DTOF_RET dtof_timestamp_calibrate_init(void)
{
    dtof_timestamp_calib_t *ts = get_timestamp();

    dtof_memset(ts, 0, sizeof(dtof_timestamp_calib_t));

    // 初始化默认参数
    ts->timer_period = DTOF_TS_TIMER_PERIOD_US;
    ts->vcsel_trig_cycle = DTOF_TS_VCSEL_CYCLE_DEFAULT;
    ts->run_flag = DTOF_FALSE;
    ts->ts_calib_k = dtof_from_float(1.0f);
    ts->ts_calib_b = dtof_from_float(0.0f);
    ts->last_calib_k = dtof_from_float(1.0f);

    return DTOF_RET_SUCCESS;
}

DTOF_RET dtof_timestamp_calibrate_calculate_float(
    dtof_real32_t in_dis,
    dtof_real32_t *out_dis)
{
    dtof_timestamp_calib_t *ts = get_timestamp();

    DTOF_CHECK_PARAM(out_dis, "输出参数为空");

    // 检查K值是否接近0
    if (dtof_from_float(ts->ts_calib_k) <= dtof_from_float(DTOF_EPSINON)) {
        *out_dis = dtof_add(in_dis, ts->ts_calib_b);
    } else {
        *out_dis = dtof_add(
            dtof_mul(in_dis, ts->ts_calib_k),
            ts->ts_calib_b
        );
    }

    return DTOF_RET_SUCCESS;
}

DTOF_RET dtof_timestamp_calibrate_calculate_int16(
    int16_t in_dis,
    int16_t *out_dis)
{
    dtof_timestamp_calib_t *ts = get_timestamp();
    dtof_real32_t result;

    DTOF_CHECK_PARAM(out_dis, "输出参数为空");

    // 检查K值是否接近0
    if (dtof_from_float(ts->ts_calib_k) <= dtof_from_float(DTOF_EPSINON)) {
        result = dtof_from_int(in_dis + dtof_to_int(ts->ts_calib_b));
    } else {
        result = dtof_add(
            dtof_mul(dtof_from_int(in_dis), ts->ts_calib_k),
            ts->ts_calib_b
        );
    }

    *out_dis = (int16_t)dtof_to_int(result);
    return DTOF_RET_SUCCESS;
}

DTOF_RET dtof_timestamp_calibrate_deinit(void)
{
    return DTOF_RET_SUCCESS;
}

DTOF_RET dtof_timestamp_calibrate_start_count(void)
{
    // dtof_timestamp_calib_t *timestamp_t = get_timestamp();


    return DTOF_RET_SUCCESS;
}

DTOF_RET dtof_timestamp_calibrate_stop_count(void)
{
    // dtof_timestamp_calib_t *timestamp_t = get_timestamp();

    return DTOF_RET_SUCCESS;
}

DTOF_RET dtof_timestamp_calibrate_process(uint16_t flashn)
{
    // dtof_timestamp_calib_t *timestamp_t = get_timestamp();
    if (0 == flashn)
    {
        return DTOF_RET_ERROR;
    }

    return DTOF_RET_SUCCESS;
}

DTOF_RET dtof_timestamp_calibrate_block(uint32_t time_out_ms)
{
    // dtof_timestamp_calib_t *timestamp_t = get_timestamp();

    return DTOF_RET_SUCCESS;
}

DTOF_RET dtof_timestamp_calibrate_update_vcsel_trig_cycle(void)
{
    // dtof_timestamp_calib_t *timestamp_t = get_timestamp();

    return DTOF_RET_SUCCESS;
}

// TODO:@liuzihao 为什么重定义
// DTOF_RET dtof_timestamp_calibrate_calculate_float(dtof_real32_t in_dis, dtof_real32_t *out_dis)
// {
//     dtof_timestamp_calib_t *timestamp_t = get_timestamp();

//     if (!out_dis)
//     {
//         return DTOF_RET_ERROR;
//     }
//
//     if ((dtof_from_float(DTOF_EPSINON) >= timestamp_t->ts_calib_k) &&
//         (dtof_from_float(-DTOF_EPSINON) <= timestamp_t->ts_calib_k))
//     {
//         *out_dis = dtof_add(in_dis, timestamp_t->ts_calib_b);
//     }
//     else
//     {
//         *out_dis = dtof_add(dtof_mul(in_dis, timestamp_t->ts_calib_k), timestamp_t->ts_calib_b);
//     }
//     return DTOF_RET_SUCCESS;
// }

// TODO:@liuzihao 为什么重定义
// DTOF_RET dtof_timestamp_calibrate_calculate_int16(int16_t in_dis, int16_t *out_dis)
// {
//     dtof_timestamp_calib_t *timestamp_t = get_timestamp();
//     if (!out_dis)
//     {
//         return DTOF_RET_ERROR;
//     }
//
//     if ((dtof_from_float(DTOF_EPSINON) >= timestamp_t->ts_calib_k) &&
//         (dtof_from_float(-DTOF_EPSINON) <= timestamp_t->ts_calib_k))
//     {
//         *out_dis = (int16_t)(in_dis + dtof_to_int(timestamp_t->ts_calib_b));
//     }
//     else
//     {
//         *out_dis =
//             (int16_t)dtof_to_int((dtof_mul(dtof_from_int(in_dis), timestamp_t->ts_calib_k) + timestamp_t->ts_calib_b));
//     }
//     return DTOF_RET_SUCCESS;
// }

DTOF_RET dtof_get_timestamp_calibrate_param(dtof_real32_t *k, dtof_real32_t *b)
{
    dtof_timestamp_calib_t *timestamp_t = get_timestamp();
    if (!k || !b)
    {
        return DTOF_RET_ERROR;
    }


    *k = timestamp_t->ts_calib_k;
    *b = timestamp_t->ts_calib_b;
    return DTOF_RET_SUCCESS;
}
#else
DTOF_RET dtof_timestamp_calibrate_init(void)
{
    return DTOF_RET_SUCCESS;
}
DTOF_RET dtof_timestamp_calibrate_deinit(void)
{
    return DTOF_RET_SUCCESS;
}
DTOF_RET dtof_timestamp_calibrate_start_count(void)
{
    return DTOF_RET_SUCCESS;
}
DTOF_RET dtof_timestamp_calibrate_stop_count(void)
{
    return DTOF_RET_SUCCESS;
}
DTOF_RET dtof_timestamp_calibrate_process( uint16_t flashn)
{
    return DTOF_RET_SUCCESS;
}
DTOF_RET dtof_timestamp_calibrate_block(uint32_t time_out_ms)
{
    return DTOF_RET_SUCCESS;
}
DTOF_RET dtof_timestamp_calibrate_update_vcsel_trig_cycle(void)
{
    return DTOF_RET_SUCCESS;
}
DTOF_RET dtof_timestamp_calibrate_calculate_float(dtof_real32_t in_dis, dtof_real32_t *out_dis)
{
    return DTOF_RET_SUCCESS;
}
DTOF_RET dtof_timestamp_calibrate_calculate_int16(int16_t in_dis, int16_t *out_dis)
{
    return DTOF_RET_SUCCESS;
}
DTOF_RET dtof_get_timestamp_calibrate_param(dtof_real32_t *k, dtof_real32_t *b)
{
    return DTOF_RET_SUCCESS;
}
#endif
