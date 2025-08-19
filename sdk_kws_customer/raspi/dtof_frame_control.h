#ifndef DTOF_FRAME_CONTROL_H
#define DTOF_FRAME_CONTROL_H

typedef enum
{
    DTOF_DSP_FIFO_INNER_MCU = 0,
    DTOF_DSP_FIFO_OUTER_MCU,
    DTOF_DISTANCE_INNER_MCU,
    DTOF_DISTANCE_OUTER_MCU,
    DTOF_MUL_CP_INNER_MCU,
    DTOF_MUL_CP_OUTER_MCU,
    DTOF_MUL_POS_INNER_MCU,
    DTOF_MUL_POS_OUTER_MCU,
    DTOF_MUL_POS_ALGO_INNER_MCU,
    DTOF_MUL_POS_ALGO_OUTER_MCU,
    DTOF_MUL_POS_2X4,
    DTOF_MUL_POS_2X2,
    DTOF_DSP_FIFO_INNER_MCU_LWO_POWER,
    FRED_DEBUG,
    DTOF_DSP_FIFO_INNER_MCU_LWO_POWER_DEBUG,
    DTOF_MODE_MAX = 32 // 目前 mode type 使用 u32, 最多有32个bit, 超出的话考虑使用u64
} dtof_mode_enum_t;

typedef enum
{
    DTOF_FARME_START = 0,
    DTOF_FARME_STOP,
    DTOF_FARME_QUIT,
    DTOF_FRAME_CONTROL_MAX = 32 // 目前 frame control 使用 u32, 最多有32个bit, 超出的话考虑使用u64
} dtof_frame_control_t;

#define DTOF_START_FRAME_FLAG 0x7788
#define DTOF_STOP_FRAME_FLAG  0x6688
#define DTOF_QUIT_FRAME_FLAG  0x1234

uint32_t get_dtof_mode_type(void);
void set_dtof_mode_type(uint32_t value);
uint32_t get_dtof_run_state(void);
void set_dtof_run_state(uint32_t value);

#endif
