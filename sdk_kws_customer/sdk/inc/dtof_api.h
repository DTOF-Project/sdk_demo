/**
 * @file dtof_api.h
 * @brief DTOF API接口定义
 * @author liuzihao
 * @date 2024/8/5
 */

#ifndef _DTOF_API_H_
#define _DTOF_API_H_

#include "inc/dtof_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(1)
// 距离测量结果结构
typedef struct {
    dtof_uint16_t frame_id;          // 帧ID
    dtof_int16_t first_target;      // 第一目标距离
    dtof_uint16_t first_intensity;   // 第一目标强度
    dtof_uint16_t main_nflash;       // 主闪光
    dtof_real32_t ambient;           // 环境光
    dtof_uint16_t reserved[6];       // 保留
} dtof_distance_result_t;

typedef struct {
    dtof_bool_t  chip_is_init;
#ifdef DTOF_POLLING_MODE
    dtof_uint16_t first_frame;
    dtof_uint16_t frame_id_pre;
#endif
dtof_uint16_t ft_calibration_type
} dtof_device_info_t;

typedef struct {
    dtof_uint16_t cg_data[CROSS_TALK_OTP_NUM]; //34
    dtof_uint16_t distance_k;
    dtof_uint16_t distance_b;
    dtof_uint16_t ref_spad;
    dtof_uint16_t bin_offset;
} dtof_ft_data_t;

typedef struct {
    dtof_uint16_t ft_calibration_type;
    dtof_ft_data_t dtof_ft_data;
} dtof_ft_cali_param_t;



#pragma pack()

// 开始测距
#define DTOF_START_DISATNCE_MODE 0X7788
// 停止测距
#define DTOF_STOP_DISATNCE_MODE 0X6688
// 退出测距
#define DTOF_QUIT_DISATNCE_MODE 0X1234
// 开始做FT校准
#define DTOF_FT_CAL_START_MODE 0X5588
// 重新加载FT数据
#define DTOF_RELOAD_FT_DATA_MODE 0X4488
// MCU睡眠指令
#define DTOF_SET_MCU_SLEEP_MODE 0X3388
// MCU睡眠FLAG
#define DTOF_SET_MCU_SLEEP_VALUE 0x17b9
#define DTOF_SLEEP_FLAG  0xFD
#define DTOF_WAKEUP_FLAG 0xDF

// 距离测量结果长度
#define DTOF_DISTANCE_RESULT_LEN (sizeof(dtof_distance_result_t) / sizeof(dtof_uint16_t))
// 上报距离(distance / 8 - 100)为真实距离
#define DTOF_DISTANCE_RESULT_DIVISOR  8
#define DTOF_DISTANCE_RESULT_OFFSET   100
// 上报第一目标强度/2为真实第一目标强度
#define DTOF_DISTANCE_INTENSITY_SHIFT 1
// 上报环境光/32为真实环境光
#define DTOF_MAIN_NOISE_DIV 32

// MCU状态定义
#define DTOF_MCU_STATE_WAKEUP         0x00
#define DTOF_MCU_STATE_SLEEP_DIRECT   DTOF_CMD_MCU_SLEEP_DIR

// IO电压配置
typedef enum {
    DTOF_IO_VOLTAGE_AUTO = 0x00,
    DTOF_IO_VOLTAGE_1V2  = 0x01,
    DTOF_IO_VOLTAGE_1V8  = 0x02,
    DTOF_IO_VOLTAGE_3V3  = 0x03
} dtof_io_voltage_t;

// FIFO数据索引定义
typedef enum {
    DTOF_FIFO_FRAME_ID = 0,         // 帧ID
    DTOF_FIFO_DISTANCE0,            // 距离0
    DTOF_FIFO_INTENSITY0,           // 强度0
    DTOF_FIFO_DISTANCE1,            // 距离1
    DTOF_FIFO_INTENSITY1,           // 强度1
    DTOF_FIFO_DISTANCE2,            // 距离2
    DTOF_FIFO_INTENSITY2,           // 强度2
    DTOF_FIFO_MAIN_NFLASH,          // 主闪光
    DTOF_FIFO_NOISE_LOW,            // 噪声低字节
    DTOF_FIFO_NOISE_HIGH            // 噪声高字节
} dtof_fifo_index_t;

// 系统错误码定义
typedef enum {
    DTOF_ERROR_OTP_CPT_CHECK_FAILED = 0,    // OTP CPT校验失败
    DTOF_ERROR_OTP_FT_CHECK_FAILED,         // OTP FT校验失败
    DTOF_ERROR_OTP_USER_CHECK_FAILED,       // OTP用户模式校验失败
    DTOF_ERROR_WDT_RESET,                   // WDT复位
    DTOF_ERROR_REG05_RESET,                 // 寄存器05复位
    DTOF_ERROR_EYE_SAFETY,                  // 眼睛安全错误
    DTOF_ERROR_OTP_CPA_CHECK_FAILED,        // OTP CPA校验失败
    DTOF_ERROR_SMOKE_STAGE1                 // 烟雾阶段1错误
} dtof_error_code_t;

// API函数声明
void dtof_init_device_info(void);
DTOF_RET dtof_get_distance_result(dtof_distance_result_t* result_info);
#ifdef DTOF_POLLING_MODE
DTOF_RET dtof_get_distance_result_polling(dtof_distance_result_t* result_info_p, dtof_bool_t *is_new_frame);
#endif
DTOF_RET dtof_write_reg_running(dtof_uint16_t reg_addr, dtof_uint16_t reg_data);
DTOF_RET dtof_read_reg_running(dtof_uint16_t reg_addr, dtof_uint16_t *reg_data);
DTOF_RET dtof_get_uuid(dtof_uint8_t *uuid, dtof_uint8_t len);
DTOF_RET dtof_set_mcu_status(dtof_uint32_t status);
DTOF_RET dtof_set_mcu_status_ram(dtof_uint32_t status);
DTOF_RET dtof_get_error_info(dtof_uint32_t *status);
DTOF_RET dtof_clear_error_info(void);
DTOF_RET dtof_clear_eye_safety_error_info(void);
DTOF_RET dtof_set_io_voltage(dtof_uint8_t value);
DTOF_RET dtof_ram_code_burn(const dtof_uint16_t *ram_code, dtof_uint16_t len, dtof_uint8_t mode, dtof_uint16_t ram_offset);
DTOF_RET dtof_sensor_init(void);
DTOF_RET dtof_start_distance_measure(void);
DTOF_RET dtof_stop_distance_measure(void);
DTOF_RET dtof_quit_distance_measure(void);
DTOF_RET dtof_start_ft_calibrate(void);
DTOF_RET dtof_reload_ft_data(void);
DTOF_RET dtof_set_ft_data(dtof_uint16_t *dtof_calibrate_data_ft_p);
const char* dtof_get_sdk_version(void);
dtof_uint16_t dtof_get_chip_version(void);

#ifdef __cplusplus
}
#endif

#endif // _DTOF_API_H_