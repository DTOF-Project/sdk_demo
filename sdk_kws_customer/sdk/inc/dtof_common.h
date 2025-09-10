/**
 * @file dtof_common.h
 * @brief DTOF公共定义
 * @author liuzihao
 * @date 2024/8/5
 */

#ifndef _DTOF_COMMON_H_
#define _DTOF_COMMON_H_

#include <stdint.h>
#include "inc/dtof_global_config.h"
#include "inc/dtof_base_type.h"

#ifdef __cplusplus
extern "C" {
#endif

// 基础寄存器定义
#define DTOF_CHIP_ID_REG_ADDR        0x00
#define DTOF_IO_CTRL_REG_ADDR        0x05
#define DTOF_SAVE_RESULT_REG_ADDR    0x50
#define DTOF_TRIGGER_FLAG_REG_ADDR   0x5D
#define DTOF_CHECK_UPGRADE_REG_ADDR  0x64
#define DTOF_ERROR_INFO_REG_ADDR     0x6E
#define DTOF_CHECK_CRC_REG_ADDR      0x6F
#define DTOF_RESET_REG_ADDR          0x75
#define DTOF_FRAME_CONTROL_REG       0xCA
#define DTOF_READ_RAM_START_REG_ADDR 0xFF
#define XTALK_DATA_SIZE 18

#define DTOF_REG251 251
#define DTOF_REG254 254
#define DTOF_REG255 255

// 系统常量定义
#define DTOF_MAX_RETRY_COUNT        3
#define DTOF_INVALID_REG_VALUE      0xDEAF
#define DTOF_RAM_START_ADDR         0x2000

#define DTOF_SWB_TYPE_FROM_RAM_FIXADDR  0x01
#define DTOF_SWB_TYPE_FROM_RAM_ANYADDR  0x02

#define DTOF_VAL_SYS_ERROR_CLR             0x00
#define DTOF_VAL_SYS_ERROR_GET             0x01
#define DTOF_VAL_SYS_ERROR_EYE_SAFETY_CLR  0x02

// 命令定义
typedef enum {
    DTOF_CMD_NONE              = 0x00,
    DTOF_CMD_SWITCH_VERSION    = 0x03,
    DTOF_CMD_READ_REG         = 0x0A,
    DTOF_CMD_WRITE_REG_ADDR   = 0x0B,
    DTOF_CMD_WRITE_REG_LOW    = 0x0C,
    DTOF_CMD_WRITE_REG_HIGH   = 0x0D,
    DTOF_CMD_SYS_ERROR        = 0x0E,
    DTOF_CMD_MCU_SLEEP_INDIR  = 0x12,
    DTOF_CMD_MCU_SLEEP_DIR    = 0x3E,
    DTOF_CMD_SET_VCCIO        = 0x17,
    DTOF_CMD_GET_UUID         = 0x19
} dtof_cmd_t;

// 命令值定义
typedef enum {
    DTOF_VAL_ERROR_CLR        = 0x00,
    DTOF_VAL_ERROR_GET        = 0x01,
    DTOF_VAL_EYE_SAFETY_CLR   = 0x02,
    DTOF_VAL_RAM_FIXED_ADDR   = 0x01,
    DTOF_VAL_RAM_ANY_ADDR     = 0x02
} dtof_cmd_val_t;

// 位操作宏优化
#define DTOF_BIT_MAX              0xFFFF
#define DTOF_BIT_POS_MAX         15

#define DTOF_CLEAR(ofs)                          (ofs = 0)
#define DTOF_BIT(ofs)                            (0x1UL << (ofs))

// 单位操作
#define DTOF_BIT_GET(reg, pos)    (((reg) >> (pos)) & 0x1)
#define DTOF_BIT_SET(reg, pos)    ((reg) |= (1UL << (pos)))
#define DTOF_BIT_CLR(reg, pos)    ((reg) &= ~(1UL << (pos)))
#define DTOF_BIT_FLIP(reg, pos)   ((reg) ^= (1UL << (pos)))
#define DTOF_BIT_WRITE(reg, pos, val) do { \
    DTOF_BIT_CLR(reg, pos); \
    (reg) |= (((val) & 0x1) << (pos)); \
} while(0)
#define DTOF_BIT_CHECK(a, b)  (!!((a) & (1ULL << (b))))

// 位掩码操作宏定义
#define DTOF_MASK_SET(x, mask)         ((x) |= (mask))
#define DTOF_MASK_CLEAR(x, mask)       ((x) &= (~(mask)))
#define DTOF_MASK_FLIP(x, mask)        ((x) ^= (mask))
#define DTOF_MASK_CHECK_ALL(x, mask)   (!(~(x) & (mask)))
#define DTOF_MASK_CHECK_ANY(x, mask)   ((x) & (mask))


// 多位操作
#define DTOF_BITS_MASK(start, end) \
    ((DTOF_BIT_MAX << (start)) & (DTOF_BIT_MAX >> (DTOF_BIT_POS_MAX - (end))))

#define DTOF_BITS_GET(reg, start, end) \
    (((reg) & DTOF_BITS_MASK(start, end)) >> (start))

#define DTOF_BITS_SET(reg, start, end) \
    ((reg) |= DTOF_BITS_MASK(start, end))

#define DTOF_BITS_CLR(reg, start, end) \
    ((reg) &= ~DTOF_BITS_MASK(start, end))

#define DTOF_BITS_WRITE(reg, start, end, val) do { \
    DTOF_BITS_CLR(reg, start, end); \
    (reg) |= (((val) << (start)) & DTOF_BITS_MASK(start, end)); \
} while(0)

// 访问权限定义
#ifdef __cplusplus
    #define DTOF_READ_ONLY     volatile const
#else
    #define DTOF_READ_ONLY     volatile const
#endif
#define DTOF_WRITE_ONLY    volatile
#define DTOF_READ_WRITE    volatile

// 弱定义属性
#ifndef DTOF_WEAK
    #define DTOF_WEAK __attribute__((weak))
#else
    #define DTOF_WEAK __WEAK
#endif

#ifdef __cplusplus
}
#endif

#endif // _DTOF_COMMON_H_
