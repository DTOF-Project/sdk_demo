#include "inc/dtof_base_type.h"

dtof_int32_t div_round_i32(dtof_int32_t value, dtof_int32_t div)
{
    if (div == 0) {
        return 0;
    }

    // 用 dtof_int64_t 防止溢出(INT_MIN / 乘2等)
    dtof_int64_t a = value;
    dtof_int64_t b = div;

    // 统一除数为正
    if (b < 0) {
        a = -a;
        b = -b;
    }

    dtof_int64_t q = a / b;   // 向 0 截断
    dtof_int64_t r = a % b;   // 与 a 同号

    if (r == 0) {
        return (dtof_int32_t)q;
    }

    dtof_int64_t ar = (r >= 0) ? r : -r;

    // 等效：|r| >= b/2  → 进 1（远离 0）
    if (ar * 2 >= b) {
        q += (a > 0) ? 1 : -1;
    }

    return (dtof_int32_t)q;
}

dtof_int16_t div_round_i16(dtof_int16_t value, dtof_int16_t div)
{
    if (div == 0) {
        return 0;
    }

    dtof_int32_t res = div_round_i32((dtof_int32_t)value,
                                    (dtof_int32_t)div);

    // 饱和（建议加，防极端情况）
    if (res > INT16_MAX) return INT16_MAX;
    if (res < INT16_MIN) return INT16_MIN;

    return (dtof_int16_t)res;
}

