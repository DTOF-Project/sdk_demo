/**
 * @file dtof_libc.h
 * @brief DTOF标准库函数封装
 * @author liuzihao
 * @date 2024/9/11
 */

#ifndef _DTOF_LIBC_H_
#define _DTOF_LIBC_H_

#include <stdarg.h>
#include "inc/dtof_base_type.h"

#ifdef __cplusplus
extern "C" {
#endif

// 系统相关定义
#ifndef CLOCK_REALTIME
    #define CLOCK_REALTIME 0
#endif

#ifndef NULL
    #define NULL ((void*)0)
#endif
#define RTOS_STRINGS  1
// 内存操作函数
#ifdef RTOS_STRINGS
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>

    // 内存管理
    #define dtof_malloc      malloc
    #define dtof_free        free
    #define dtof_calloc      calloc
    #define dtof_memcpy      memcpy
    #define dtof_memset      memset
    #define dtof_memmove     memmove

    // 字符串操作
    #define dtof_strlen      strlen
    #define dtof_strcpy      strcpy
    #define dtof_strncpy     strncpy
    #define dtof_strcmp      strcmp
    #define dtof_strncmp     strncmp
    #define dtof_strstr      strstr
    #define dtof_strcat      strcat
    #define dtof_strncat     strncat
    #define dtof_strchr      strchr
    #define dtof_strrchr     strrchr
    #define dtof_strspn      strspn
    #define dtof_strtok      strtok

    // 类型转换
    #define dtof_strtoul     strtoul
    #define dtof_strtol      strtol
    #define dtof_atoi        atoi
    #define dtof_toupper     toupper
    #define dtof_tolower     tolower

    // 文件操作
    #define dtof_fgets       fgets
    #define dtof_fgetc       fgetc
    #define dtof_fputs       fputs
    #define dtof_fputc       fputc
    #define dtof_fwrite      fwrite
    #define dtof_fprintf     fprintf
    #define dtof_fflush      fflush
    #define dtof_vfprintf    vfprintf
    #define dtof_printf      printf

#else
    // 自定义实现声明
    void* dtof_malloc(size_t size);
    void  dtof_free(void* ptr);
    void* dtof_memcpy(void* dst, const void* src, size_t size);
    void* dtof_memset(void* dst, int value, size_t size);
    void* dtof_memmove(void* dst, const void* src, size_t size);

    // 时间相关
    uint32_t dtof_get_time_ms(void);
    DTOF_RET dtof_clock_gettime(dtof_clockid_t clock_id, struct timespec* tp);

    // GPIO操作
    DTOF_RET dtof_gpio_init(uint32_t gpio);
    DTOF_RET dtof_gpio_deinit(uint32_t gpio);
    DTOF_RET dtof_gpio_read(uint32_t gpio, uint32_t* value);
    DTOF_RET dtof_gpio_write(uint32_t gpio, uint32_t value);
#endif

#ifdef __cplusplus
}
#endif

#endif // _DTOF_LIBC_H_
