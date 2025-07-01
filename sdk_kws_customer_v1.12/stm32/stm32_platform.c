/*
 * stm32_platform.c
 *
 *  Created on: 2024/9/11
 *      Author: liuzihao
 */
#include <stdio.h>
#include <string.h>
#include <stdio.h>

#include "platform_user_config.h"

#include "base/inc/mos_platform.h"
#include "inc/util.h"
#include "board.h"
#include "stm32g4xx_hal.h"

error_key_t STM32LastError;
struct timespec g_basetime = {.tv_sec = DEFAULT_TV_SEC,
                              .tv_nsec = DEFAULT_TV_NSEC};

extern volatile uint32_t gSysTick;
extern volatile uint32_t gSysVAL;


error_key_t dtof_get_last_error() { return STM32LastError; }
void dtof_set_last_error(error_key_t err) { STM32LastError = err; }
DTOF_RET platform_uninit() { return DTOF_RET_SUCCESS; }

// device_driver
/**
 * @brief init the board
 *
 * @return DTOF_RET
 */
DTOF_RET platform_init() {
    int ret = hw_board_init();
    if(DTOF_RET_SUCCESS != ret) return ret;
    // system_timer_init();
    return DTOF_RET_SUCCESS;
}

DTOF_RET platform_deinit() {
    return hw_board_uninit();
}

uint64_t generated_us(void)
{
    uint64_t utmp = gSysTick;
    uint64_t us1 = utmp * 1000;
    uint64_t us2 = us1 + 1000 - SysTick->VAL/180;
    return us2;
}
static inline uint32_t systick_isactive_counterflag(void)
{
  return ((SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) == (SysTick_CTRL_COUNTFLAG_Msk));
}
//Return the micro seconds since the last reboot of the CPU
uint64_t get_current_us(void)
{
    /* Ensure COUNTFLAG is reset by reading SysTick control and status register */
    systick_isactive_counterflag();
    uint64_t m = HAL_GetTick();
    const uint32_t tms = SysTick->LOAD + 1;
    __IO uint32_t u = tms - SysTick->VAL;
    if (systick_isactive_counterflag()) {
        m = HAL_GetTick();
        u = tms - SysTick->VAL;
    }
    uint64_t u1 = (m * 1000 + (u * 1000) / tms);
    return u1;
}

void delay_time_us(uint64_t us) {
    uint64_t t = get_current_us() + us;
    while (get_current_us() < t) {}
    return;
}

void usleep(int micro_seconds) {
    uint64_t curTime;
    curTime = generated_us();
    while(micro_seconds + curTime > generated_us());
}


DTOF_RET clock_systimespec(struct timespec *tp) {
#ifdef HAVE_HIRES_TIMER
    uint32_t msecs;
    uint32_t secs;
    uint32_t usecs;
    uint32_t nsecs;

    msecs = s_trig_timer_count;
    secs = msecs / MSEC_PER_SEC;
    nsecs = (msecs - (secs * MSEC_PER_SEC)) * NSEC_PER_MSEC;
    tp->tv_sec = (uint32_t)secs;
    tp->tv_nsec = (uint64_t)nsecs;
    uint32_t usec_per_tick = SystemCoreClock / 1000000;
    // usecs = dtof_get_trig_timer_count();
    usecs = s_trig_timer_cnt;
    tp->tv_nsec += usecs * 1000;
#else
    uint32_t msecs;
    uint32_t secs;
    uint32_t usecs;
    uint32_t nsecs;
    systick_isactive_counterflag();
    uint32_t m = HAL_GetTick();
    const uint32_t tms = SysTick->LOAD + 1;
    __IO uint32_t u = tms - SysTick->VAL;
    if (systick_isactive_counterflag()) {
        m = HAL_GetTick();
        u = tms - SysTick->VAL;
    }
    secs = m / MSEC_PER_SEC;
    msecs = (m - (secs * MSEC_PER_SEC));
    tp->tv_sec = (uint32_t)secs;
    tp->tv_nsec = (uint32_t)msecs * NSEC_PER_MSEC;
    usecs = u / 180;
    tp->tv_nsec += usecs * 1000;
#endif
    return DTOF_RET_SUCCESS;
}

DTOF_RET clock_settime(clockid_t clock_id, const struct timespec *tp) {
    struct timespec bias;
    int ret = DTOF_RET_SUCCESS;
    if (tp == NULL) {
        return DTOF_RET_ERROR;
    }
    if (clock_id == CLOCK_REALTIME) {
        g_basetime.tv_sec = tp->tv_sec;
        g_basetime.tv_nsec = tp->tv_nsec;
        ret = clock_systimespec(&bias);
        if (DTOF_RET_SUCCESS != ret) {
            return DTOF_RET_ERROR;
        }
        if (g_basetime.tv_nsec < bias.tv_nsec) {
            g_basetime.tv_nsec += NSEC_PER_SEC;
            g_basetime.tv_sec--;
        }
        g_basetime.tv_nsec -= bias.tv_nsec;
        g_basetime.tv_sec -= bias.tv_sec;
        /* Setup the RTC here(if required)(lo- or high-res) */

    } else {
        // POSIX just demands CLOCK_REALTIME to be present
        // dtof_set_last_error(EINVAL);
        ret = DTOF_RET_ERROR;
    }
    return ret;
}

DTOF_RET clock_gettime(clockid_t clock_id, struct timespec *tp) {
    struct timespec ts;
    uint32_t carry;
    int ret = DTOF_RET_ERROR;
    if (tp == NULL) {
        return DTOF_RET_ERROR;
    }
    if (CLOCK_MONOTONIC == clock_id) {
        ret = clock_systimespec(tp);
    } else if (CLOCK_REALTIME == clock_id) {
        ret = clock_systimespec(&ts);
        if (DTOF_RET_SUCCESS == ret) {
            ts.tv_sec += (uint32_t)g_basetime.tv_sec;
            ts.tv_nsec += (uint32_t)g_basetime.tv_nsec;
            if (ts.tv_nsec >= NSEC_PER_SEC) {
                carry = ts.tv_nsec / NSEC_PER_SEC;
                ts.tv_sec += carry;
                ts.tv_nsec -= (carry * NSEC_PER_SEC);
            }
            tp->tv_sec = ts.tv_sec;
            tp->tv_nsec = ts.tv_nsec;
        }
    } else {
        // dtof_set_last_error(EINVAL);
        ret = DTOF_RET_ERROR;
    }
    return ret;
}

int gettimeofday(struct timeval *tp, void *tzp)
{
  struct timespec ts;
  int ret = -1;
  ret = clock_gettime(CLOCK_REALTIME, &ts);
  if (ret == DTOF_RET_SUCCESS)
  {
    tp->tv_sec  = ts.tv_sec;
    tp->tv_usec = ts.tv_nsec / NSEC_PER_USEC;
  }
  return ret;
}


time_t time(time_t *calptr)
{
    struct timeval tp;
    int ret;
    ret = gettimeofday(&tp, NULL);
    if (ret == 0)
    {
        if (calptr)
        {
            *calptr = tp.tv_sec;
        }
        return tp.tv_sec;
    }
    return (time_t)-1;
}

