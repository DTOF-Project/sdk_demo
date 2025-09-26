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
    dtof_int16_t first_target;       // 第一目标距离
    dtof_uint16_t first_intensity;   // 第一目标强度
    dtof_uint16_t main_nflash;       // 主闪光
    dtof_real32_t ambient;           // 环境光
    dtof_int32_t is_legal_frame;     // 是否合法帧, 1合法, 0非法
    dtof_uint16_t reserved[8];       // 保留
} dtof_distance_result_t;

typedef struct {
    dtof_uint8_t  device_id;
    dtof_bool_t  is_init;
    dtof_uint16_t first_frame;
    dtof_uint16_t frame_id_pre;
    dtof_int32_t distance_offset;
} dtof_device_info_t;

typedef enum {
    NORMAL_DISTANCE_MODE = 0, // 正常测距模式
    DO_XTALK_CALIBRATION_MODE = 1, // 600mm处做xtalk校准
    DO_OFFSET_CALIBRATION_MODE = 2  // 20mm处做测距offset校准
} dtof_start_mode_t;

#pragma pack()

// 设备数
#define DTOF_DEVICE_0 0
#define DTOF_DEVICE_1 1
#define DTOF_MAX_DEVICE_NUM  2
// 开始测距
#define DTOF_START_DISATNCE_MODE 0X7788
// 停止测距
#define DTOF_STOP_DISATNCE_MODE 0X6688
// 退出测距
#define DTOF_QUIT_DISATNCE_MODE 0X1234

// 距离测量结果长度
#define DTOF_DISTANCE_RESULT_LEN (sizeof(dtof_distance_result_t) / sizeof(dtof_uint16_t))
// 上报距离/8为真实距离
#define DTOF_DISTANCE_RESULT_DIVISOR  8
#define DTOF_DISTANCE_RESULT_OFFSET   100
// 上报第一目标强度/2为真实第一目标强度
#define DTOF_DISTANCE_INTENSITY_SHIFT 1
// 上报环境光/32为真实环境光
#define DTOF_MAIN_NOISE_DIV 32

// MCU状态定义
#define DTOF_MCU_STATE_WAKEUP         0x00
#define DTOF_MCU_STATE_SLEEP_DIRECT   DTOF_CMD_MCU_SLEEP_DIR
#define DTOF_MCU_STATE_SLEEP_INDIRECT DTOF_CMD_MCU_SLEEP_INDIR

// UUID长度定义
#define DTOF_UUID_LENGTH              16

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
void dtof_set_distance_offset(dtof_uint8_t device_id, dtof_int32_t offset);
dtof_int32_t dtof_get_distance_offset(dtof_uint8_t device_id);
DTOF_RET dtof_get_fifo(dtof_uint8_t device_id, dtof_start_mode_t dtof_start_mode, dtof_distance_result_t* result_info_p);
DTOF_RET dtof_get_distance_result(dtof_uint8_t device_id, dtof_start_mode_t dtof_start_mode, dtof_distance_result_t* result_info_p, dtof_bool_t *is_new_frame);
DTOF_RET dtof_get_uuid(dtof_uint8_t device_id, dtof_uint8_t *uuid, dtof_uint8_t len);
DTOF_RET dtof_set_mcu_status(dtof_uint8_t device_id, dtof_uint32_t status);
DTOF_RET dtof_get_error_info(dtof_uint8_t device_id, dtof_uint32_t *status);
DTOF_RET dtof_clear_error_info(dtof_uint8_t device_id);
DTOF_RET dtof_clear_eye_safety_error_info(dtof_uint8_t device_id);
DTOF_RET dtof_set_io_voltage(dtof_uint8_t device_id, dtof_uint8_t value);
DTOF_RET dtof_ram_code_burn(dtof_uint8_t device_id, const dtof_uint16_t *ram_code,
                           dtof_uint16_t len,
                           dtof_uint8_t mode,
                           dtof_uint16_t ram_offset);
DTOF_RET dtof_init_and_wait_for_ready(dtof_uint8_t device_id, dtof_uint16_t * chip_id_p, dtof_start_mode_t dtof_start_mode);
DTOF_RET dtof_version_upgrade(dtof_uint8_t device_id, dtof_uint8_t *uuid_p, dtof_uint8_t *ft_data_p, dtof_uint16_t len);
DTOF_RET dtof_start_distance_measure(dtof_uint8_t device_id);
DTOF_RET dtof_stop_distance_measure(dtof_uint8_t device_id);
DTOF_RET dtof_quit_distance_measure(dtof_uint8_t device_id);
DTOF_RET dtof_do_distance_calibration(dtof_uint8_t device_id, dtof_int32_t *distance_offset);
const char* dtof_get_sdk_version(void);
void dtof_init_all_device_info(void);
dtof_uint16_t dtof_get_chip_version(dtof_uint8_t device_id);

#ifdef __cplusplus
}
#endif

#endif // _DTOF_API_H_