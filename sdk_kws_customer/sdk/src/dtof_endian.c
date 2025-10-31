/**
 * @file dtof_endian.c
 * @brief DTOF字节序检测实现
 * @author liuzihao
 * @date 2024/9/11
 */

#include "inc/dtof_endian.h"
#include "inc/dtof_base_type.h"

// 字节序检测状态
static volatile int g_endian_initialized = 0;
static volatile int g_host_is_big_endian = 0;

// 字节序检测魔数
#define ENDIAN_DETECT_MAGIC 0x11223344
#define ENDIAN_BE_PATTERN   0x11

static void dtof_endian_init(void)
{
    union {
        dtof_uint32_t value;
        dtof_uint8_t  bytes[4];
    } detect;

    if (!g_endian_initialized) {
        detect.value = ENDIAN_DETECT_MAGIC;
        g_host_is_big_endian = (detect.bytes[0] == ENDIAN_BE_PATTERN);
        g_endian_initialized = 1;
    }
}

int dtof_is_big_endian(void)
{
    if (!g_endian_initialized) {
        dtof_endian_init();
    }
    return g_host_is_big_endian;
}
