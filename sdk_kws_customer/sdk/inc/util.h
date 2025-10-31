/**
 * @file util.h
 * @brief 通用工具宏定义
 * @author liuzihao
 * @date 2024/9/11
 */

#ifndef _UTIL_H_
#define _UTIL_H_

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// 数组操作
#define ARRAY_SIZE(arr)     (sizeof(arr) / sizeof((arr)[0]))
#define ARRAY_EMPTY(arr)    (ARRAY_SIZE(arr) == 0)
#define ARRAY_LAST(arr)     ((arr)[ARRAY_SIZE(arr) - 1])

// 位操作
#define BIT(n)              (1UL << (n))
#define BIT_SET(x, n)       ((x) |= BIT(n))
#define BIT_CLR(x, n)       ((x) &= ~BIT(n))
#define BIT_TEST(x, n)      ((x) & BIT(n))
#define BIT_TOGGLE(x, n)    ((x) ^= BIT(n))
#define ONE_BIT_IS_SET(x)   ((x) && !((x) & ((x)-1)))

// 内存对齐
#define ALIGN_SIZE          sizeof(uint32_t)
#define ALIGN_MASK          (ALIGN_SIZE - 1)
#define ALIGN_DOWN(x)       ((uintptr_t)(x) & ~ALIGN_MASK)
#define ALIGN_UP(x)         (((uintptr_t)(x) + ALIGN_MASK) & ~ALIGN_MASK)
#define IS_ALIGNED(x)       (((uintptr_t)(x) & ALIGN_MASK) == 0)

// 最大最小值
#define MIN(a, b) ({                    \
    __typeof__(a) _a = (a);            \
    __typeof__(b) _b = (b);            \
    _a < _b ? _a : _b;                 \
})

#define MAX(a, b) ({                    \
    __typeof__(a) _a = (a);            \
    __typeof__(b) _b = (b);            \
    _a > _b ? _a : _b;                 \
})

#define CLAMP(x, min, max)  (MIN(MAX((x), (min)), (max)))

// 容器操作
#define CONTAINER_OF(ptr, type, member) ({                      \
    const __typeof__(((type *)0)->member) *__mptr = (ptr);     \
    (type *)((char *)__mptr - offsetof(type, member));         \
})

// 数值计算
#define DIV_ROUND_UP(n, d)      (((n) + (d) - 1) / (d))
#define DIV_ROUND_CLOSEST(x, d) ({                             \
    __typeof__(x) __x = x;                                     \
    __typeof__(d) __d = d;                                     \
    __typeof__(x) __result;                                    \
    if ((__x) >= 0)                                            \
        __result = ((__x) + ((__d) / 2)) / (__d);             \
    else                                                       \
        __result = ((__x) - ((__d) / 2)) / (__d);             \
    __result;                                                  \
})

// 编译器相关
#ifndef UNUSED
#define UNUSED(x)           ((void)(x))
#endif
#define PACKED             __attribute__((packed))
#define WEAK              __attribute__((weak))
#define ALIGNED(x)        __attribute__((aligned(x)))

#ifdef __cplusplus
}
#endif

#endif // _UTIL_H_
