/**
 * @file dtof_float.h
 * @brief DTOF浮点和定点数计算接口
 * @author liuzihao
 * @date 2024/9/11
 */

#ifndef _DTOF_FLOAT_H_
#define _DTOF_FLOAT_H_

#include "inc/dtof_base_type.h"


#ifdef __cplusplus
extern "C" {
#endif

// 基础数学运算函数声明
#ifdef CALC_USING_FIX1616
    // 定点数运算接口
    dtof_real32_t fix16_add(dtof_real32_t a, dtof_real32_t b);
    dtof_real32_t fix16_sub(dtof_real32_t a, dtof_real32_t b);
    dtof_real32_t fix16_mul(dtof_real32_t a, dtof_real32_t b);
    dtof_real32_t fix16_div(dtof_real32_t a, dtof_real32_t b);
    dtof_real32_t fix16_sqrt(dtof_real32_t x);
    dtof_real32_t fix16_log10(dtof_real32_t x);

    // 类型转换接口
    dtof_real32_t fix16_from_int(int a);
    dtof_real32_t fix16_from_float(float a);
    int fix16_to_int(dtof_real32_t a);

    // 统一接口映射
    #define dtof_add          fix16_add
    #define dtof_sub          fix16_sub
    #define dtof_mul          fix16_mul
    #define dtof_div          fix16_div
    #define dtof_sqrt         fix16_sqrt
    #define dtof_log10        fix16_log10
    #define dtof_from_float   fix16_from_float
    #define dtof_from_int     fix16_from_int
    #define dtof_to_int       fix16_to_int

#else
    // 浮点数运算接口
    dtof_real32_t float_add(dtof_real32_t a, dtof_real32_t b);
    dtof_real32_t float_sub(dtof_real32_t a, dtof_real32_t b);
    dtof_real32_t float_mul(dtof_real32_t a, dtof_real32_t b);
    dtof_real32_t float_div(dtof_real32_t a, dtof_real32_t b);
    dtof_real32_t float_sqrt(dtof_real32_t x);

    // 类型转换接口
    dtof_real32_t float_from_int(int a);
    dtof_real32_t float_from_float(float a);
    int float_to_int(dtof_real32_t a);

    // 统一接口映射
    #define dtof_add          float_add
    #define dtof_sub          float_sub
    #define dtof_mul          float_mul
    #define dtof_div          float_div
    #define dtof_sqrt         float_sqrt
    #define dtof_log10        log10f
    #define dtof_from_float   float_from_float
    #define dtof_from_int     float_from_int
    #define dtof_to_int       float_to_int
#endif

#ifdef __cplusplus
}
#endif

#endif // _DTOF_FLOAT_H_
