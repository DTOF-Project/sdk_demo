/**
 * @file dtof_log.h
 * @brief DTOF日志和错误处理
 */

#ifndef _DTOF_LOG_H_
#define _DTOF_LOG_H_

#include "inc/dtof_libc.h"
#include "inc/dtof_base_type.h"

#ifdef __cplusplus
extern "C" {
#endif

// 日志级别定义
typedef enum {
    DTOF_LOG_ERROR = 0,
    DTOF_LOG_WARN,
    DTOF_LOG_INFO,
    DTOF_LOG_DEBUG
} dtof_log_level_t;

#define ENABLE_LOG
#ifdef ENABLE_LOG
    #define DEBUG_LOG_FLAG
    #undef DEBUG_LOG_FLAG
    
    #define DTOF_LOG(fmt, ...) \
        dtof_printf("[DTOF] " fmt "\n", ##__VA_ARGS__)

    #define DTOF_LOG_ERR(fmt, ...) \
        dtof_printf("[DTOF-ERROR] %s:%d " fmt "\n", \
                    __FILE__, __LINE__, ##__VA_ARGS__)

    // 错误处理宏
    #define DTOF_CHECK_RET(expr, msg) do { \
        DTOF_RET __ret = (expr); \
        if (__ret != DTOF_RET_SUCCESS) { \
            DTOF_LOG_ERR("%s: %d", msg, __ret); \
            return __ret; \
        } \
    } while(0)

    #define DTOF_CHECK_RET_VOID(expr, msg) do { \
        DTOF_RET __ret = (expr); \
        if (__ret != DTOF_RET_SUCCESS) { \
            DTOF_LOG_ERR("%s: %d", msg, __ret); \
            return; \
        } \
    } while(0)

    #define DTOF_CHECK_WARN(expr, msg) do { \
        DTOF_RET __ret = (expr); \
        if (__ret != DTOF_RET_SUCCESS) { \
            DTOF_LOG_ERR("%s: %d", msg, __ret); \
        } \
    } while(0)

    // 指针检查宏
    #define DTOF_CHECK_PTR(ptr) do { \
        if (!(ptr)) { \
            DTOF_LOG_ERR("空指针错误"); \
            return DTOF_RET_NULL_PTR; \
        } \
    } while(0)

    #define DTOF_CHECK_PTR_VOID(ptr) do { \
        if (!(ptr)) { \
            DTOF_LOG_ERR("空指针错误"); \
            return; \
        } \
    } while(0)

    #define DTOF_CHECK_PARAM(cond, msg) do { \
        if (!(cond)) { \
            DTOF_LOG("%s\n", msg); \
            return DTOF_RET_ERROR; \
        } \
    } while(0)

#else
    #define DTOF_LOG(...)
    #define DTOF_LOG_ERR(...)

    #define DTOF_CHECK_RET(expr, msg) do { \
        DTOF_RET ret = (expr); \
        if (ret != DTOF_RET_SUCCESS) return ret; \
    } while(0)

    #define DTOF_CHECK_RET_VOID(expr, msg) do { \
        if ((expr) != DTOF_RET_SUCCESS) return; \
    } while(0)

    #define DTOF_CHECK_WARN(expr, msg)

    #define DTOF_CHECK_PTR(ptr) do { \
        if (!(ptr)) return DTOF_RET_NULL_PTR; \
    } while(0)

    #define DTOF_CHECK_PTR_VOID(ptr) do { \
        if (!(ptr)) return; \
    } while(0)
#endif

#ifdef __cplusplus
}
#endif

#endif // _DTOF_LOG_H_