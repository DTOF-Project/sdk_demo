/**
 * @file dtof_base_type.h
 * @brief DTOF基础类型定义
 * @author liuzihao
 * @date 2024/9/11
 */

#ifndef _DTOF_BASE_TYPE_H_
#define _DTOF_BASE_TYPE_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// 平台相关类型定义
#ifdef __linux__
    #include <unistd.h>
    #include <pthread.h>
    #include <time.h>
    // time.h
    #include <bits/types/clockid_t.h>
#elif defined(_MSC_VER)
    #include <time.h>
    #include <BaseTsd.h>
    #include <windows.h>
    #include <assert.h>
    typedef SSIZE_T ssize_t;
    typedef uint32_t clockid_t;
#else
    typedef uint32_t clockid_t;
    typedef uint32_t time_t;
    typedef int32_t ssize_t;
#endif

// 基础数据类型定义
typedef int             error_key_t;
typedef int8_t         dtof_int8_t;
typedef int16_t        dtof_int16_t;
typedef int32_t        dtof_int32_t;
typedef int64_t        dtof_int64_t;
typedef uint8_t        dtof_uint8_t;
typedef uint16_t       dtof_uint16_t;
typedef uint32_t       dtof_uint32_t;
typedef uint64_t       dtof_uint64_t;
typedef int32_t        dtof_fix1616_t;
typedef int32_t        dtof_fix2408_t;
typedef int32_t        dtof_fix2012_t;
typedef dtof_int8_t    dtof_bool_t;

// 系统类型定义
typedef time_t         dtof_time_t;
typedef ssize_t        dtof_ssize_t;
typedef clockid_t      dtof_clockid_t;
typedef uint32_t       dtof_tick_t;
typedef int32_t        DTOF_RET;
typedef dtof_int32_t   dev_handle_t;

#ifndef NULL
#define NULL ((void*)0)
#endif

// 浮点数类型定义
#ifdef CALC_USING_FIX1616
    typedef dtof_int32_t dtof_real32_t;
#else
    typedef float dtof_real32_t;
#endif

// 常量定义
#define DTOF_SUCCESS   ((dtof_bool_t)(0))
#define DTOF_FAIL      ((dtof_bool_t)(1))
#define DTOF_TRUE      ((dtof_bool_t)(1))
#define DTOF_FALSE     ((dtof_bool_t)(0))
#define DTOF_EPSILON   (0.000001f)


// 错误码定义
typedef enum {
    DTOF_RET_OK = 0,                    // 成功
    DTOF_RET_ERROR = -1,                // 通用错误
    DTOF_RET_NULL_PTR = -2,             // 空指针
    DTOF_RET_INIT_FAILED = -3,          // 初始化失败
    DTOF_RET_INVALID_PARAM = -4,        // 无效参数
    DTOF_RET_DEVICE_ERROR = -5,         // 设备错误
    DTOF_RET_GPIO_INIT_ERROR = -6,      //  _GPIO_INIT_ERROR
    DTOF_RET_COMM_INIT_ERROR = -7,      // _COMM_INIT_ERROR
    // ... 其他错误码保持不变 ...
    DTOF_RET_LIMIT = -0xFF              // 错误码边界
} dtof_error_t;

// 返回值宏定义
#define DTOF_RET_SUCCESS    DTOF_RET_OK
#define DTOF_RET_FAILED     DTOF_RET_ERROR

// 设备句柄类型定义
typedef void* DTOF_SPI_Handle_t;
typedef void* DTOF_I2C_Handle_t;
typedef void* DTOF_UART_Handle_t;
typedef void* DTOF_UART_DMA_Handle_t;
typedef void* DTOF_ADC_Handle_t;

#ifdef __cplusplus
}
#endif

#endif // _DTOF_BASE_TYPE_H_
