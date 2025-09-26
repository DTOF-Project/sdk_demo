/**
 * @file dtof_endian.h
 * @brief DTOF字节序处理
 * @author liuzihao
 * @date 2024/9/11
 */

#ifndef _DTOF_ENDIAN_H_
#define _DTOF_ENDIAN_H_

#include "inc/dtof_base_type.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 检查主机是否为大端序
 * @return 0-小端序, 1-大端序
 */
int dtof_is_big_endian(void);

/**
 * @brief 16位字节序转换
 */
#define DTOF_SWAP16(val) ((((val) & 0x00FF) << 8) | (((val) & 0xFF00) >> 8))

/**
 * @brief 主机字节序转大端序(16位)
 * @param dst 目标地址
 * @param src 源地址
 */
#define DTOF_HOST_TO_BE16(dst, src) do { \
    dtof_uint8_t *_dst = (dtof_uint8_t *)(dst); \
    dtof_uint16_t _val = *((dtof_uint16_t *)(src)); \
    _dst[0] = (_val >> 8) & 0xFF; \
    _dst[1] = _val & 0xFF; \
} while (0)

/**
 * @brief 主机字节序转小端序(16位)
 * @param dst 目标地址
 * @param src 源地址
 */
#define DTOF_HOST_TO_LE16(dst, src) do { \
    dtof_uint8_t *_dst = (dtof_uint8_t *)(dst); \
    dtof_uint16_t _val = *((dtof_uint16_t *)(src)); \
    _dst[0] = _val & 0xFF; \
    _dst[1] = (_val >> 8) & 0xFF; \
} while (0)

/**
 * @brief 大端序转主机字节序(16位)
 */
#define DTOF_BE16_TO_HOST(val) \
    (dtof_is_big_endian() ? (val) : DTOF_SWAP16(val))

/**
 * @brief 小端序转主机字节序(16位)
 */
#define DTOF_LE16_TO_HOST(val) \
    (dtof_is_big_endian() ? DTOF_SWAP16(val) : (val))

#ifdef __cplusplus
}
#endif

#endif // _DTOF_ENDIAN_H_
