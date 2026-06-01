# DTOF SDK 移植与使用指南

## 1. 文档说明

本文档提供了 SDK 移植详细步骤、主要 API 以及典型使用流程，面向 SDK 集成与应用开发人员。
该 SDK 用于控制和获取 ToF 传感器的测距数据。

---

## 2. 开发环境准备

### 2.1 硬件要求

ToF 传感器模块
目标微控制器开发板（如 STM32）
调试器（如 ST-Link）

### 2.2 软件要求

集成开发环境（如 Keil MDK、IAR Workbench 或 STM32CubeIDE）
SDK 源代码包

### 2.3 SDK 文件结构

```
soc/
└─ sdk/
   ├─ diag/
   │  │  dtof_diag.c
   ├─ inc/
   │  ├─ lib/
   │  │   ├─ dtof_ft.h
   │  ├─ dtof_api.h
   │  ├─ dtof_driver.h
   │  └─ ...
   ├─ src/
   │  ├─ lib/
   │  │   ├─ libdtof_ft.so
   │  ├─ dtof_api.c
   │  ├─ dtof_driver.c
   │  ├─ dtof_calibration_ft.c
   │  └─ ...
```

目录说明：

- `inc/`：SDK 对外头文件
- `src/`：SDK 核心实现
- `src/lib/libdtof_ft.so`：ft校准相关接口，若客户平台/工具链不同，需要重新提供适配版本，目前这版使用 `aarch64-buildroot-linux-gnu-gcc` 工具链

---

## 3. 移植步骤

### 3.1 添加 SDK 源文件到项目

将 sdk 下的 .c 文件添加到您的项目中
将 sdk/src/lib 下的 so 文件添加到您的项目中

### 3.2 添加 SDK 头文件路径到项目包含路径中

将 sdk/ 目录添加到项目包含路径中

---

## 4. 用户需要实现的底层接口

SDK 不直接绑定具体硬件平台，用户需要根据自身 MCU 平台实现 `dtof_driver.h` 中定义的底层接口。

### 4.1 寄存器访问接口

```c
DTOF_RET dtof_reg_burst_read(dtof_uint8_t reg_addr, dtof_uint16_t *reg_data_p, dtof_uint16_t len);
DTOF_RET dtof_reg_burst_write(dtof_uint8_t reg_addr, dtof_uint16_t *reg_data_p, dtof_uint16_t len);
DTOF_RET dtof_reg_burst_write_burn(dtof_uint8_t reg_addr, const dtof_uint16_t *reg_data_p, dtof_uint16_t len);
```

接口职责：

- `dtof_reg_burst_read()`：批量读取设备寄存器
- `dtof_reg_burst_write()`：批量写入设备寄存器
- `dtof_reg_burst_write_burn()`：在设备初始化与固件加载阶段执行批量写入
- 如使用 iic 接口，`iic device addr = 0x41`

### 4.2 交互与延时接口

```c
void dtof_sleep_ms(dtof_uint32_t time);
```

接口职责：

- `dtof_sleep_ms()`：提供毫秒级延时能力，供初始化、切模式、校准等流程使用

### 4.3 中断辅助接口

```c
void dtof_set_interrupt_flag(dtof_bool_t flag);
dtof_bool_t dtof_get_interrupt_flag(void);
```

说明：

- 若使用中断模式，应由用户在中断回调中维护该标志
- 若使用轮询模式，可提供空实现

### 4.4 FT 数据持久化接口

```c
DTOF_RET dtof_get_ft_data_from_flash_multi_mode(dtof_uint16_t *ft_data, dtof_uint16_t len, dtof_run_mode_e run_mode, dtof_bool_t *is_legal_data);
DTOF_RET dtof_set_ft_data_to_flash_multi_mode(dtof_uint16_t *ft_data, dtof_uint16_t len, dtof_run_mode_e run_mode);
```

- `is_legal_data` 用于标识该模式 FT 数据是否有效

---

## 5. 底层接口实现要求与示例

以下示例用于说明接口实现思路，用户应根据实际硬件平台进行修改。

### 5.1 I2C 读写示例

```c
#define DTOF_I2C_ADDR 0x41

DTOF_RET dtof_reg_burst_read(dtof_uint8_t reg_addr, dtof_uint16_t *reg_data_p, dtof_uint16_t len)
{
    uint16_t byte_len = len * 2;

    if (platform_i2c_mem_read(DTOF_I2C_ADDR, reg_addr, (uint8_t *)reg_data_p, byte_len) != 0)
    {
        return DTOF_RET_FAILED;
    }

    dtof_convert_endian(reg_data_p, len);
    return DTOF_RET_SUCCESS;
}

DTOF_RET dtof_reg_burst_write(dtof_uint8_t reg_addr, dtof_uint16_t *reg_data_p, dtof_uint16_t len)
{
    uint16_t byte_len = len * 2;

    dtof_convert_endian(reg_data_p, len);
    if (platform_i2c_mem_write(DTOF_I2C_ADDR, reg_addr, (uint8_t *)reg_data_p, byte_len) != 0)
    {
        dtof_convert_endian(reg_data_p, len);
        return DTOF_RET_FAILED;
    }
    dtof_convert_endian(reg_data_p, len);

    return DTOF_RET_SUCCESS;
}

DTOF_RET dtof_reg_burst_write_burn(dtof_uint8_t reg_addr, const dtof_uint16_t *reg_data_p, dtof_uint16_t len)
{
    uint16_t byte_len = len * 2;

    if (platform_i2c_mem_write(DTOF_I2C_ADDR, reg_addr, (const uint8_t *)reg_data_p, byte_len) != 0)
    {
        return DTOF_RET_FAILED;
    }

    return DTOF_RET_SUCCESS;
}

```

说明：

- `platform_i2c_mem_read()` / `platform_i2c_mem_write()` 由用户平台提供
- 若平台使用 SPI，可按相同方式封装到 SDK 接口中
- 若底层接口已处理字节序，可根据实际情况决定是否保留 `dtof_convert_endian()`
- `dtof_reg_burst_write_burn` 和 `dtof_reg_burst_write` 的区别是：
  `dtof_reg_burst_write` 中做了大小端转换，而 `dtof_reg_burst_write_burn` 没有，此举是为了节省栈空间

### 5.2 延时函数实现示例

```c
void dtof_sleep_ms(dtof_uint32_t time)
{
    platform_delay_ms(time);
}
```

要求：

- 延时单位必须为毫秒
- 该接口会被初始化、模式切换、校准等流程使用

### 5.3 FT 数据读写实现示例

```c
DTOF_RET dtof_get_ft_data_from_flash_multi_mode(dtof_uint16_t *ft_data, dtof_uint16_t len, dtof_run_mode_e run_mode, dtof_bool_t *is_legal_data);
DTOF_RET dtof_set_ft_data_to_flash_multi_mode(dtof_uint16_t *ft_data, dtof_uint16_t len, dtof_run_mode_e run_mode);
```

这两个接口中
- 要求 ft 数据掉电可保存
- `run_mode` 可能的取值为 0, 1, 2，分别对应 30HZ, 120HZ LP, 120HZ LLP 模式
- 每个模式 `ft_data` 的长度为 78 bytes (`sizeof(dtof_ft_cali_param_t)`)，也就是存储 ft 数据最少需要 78 * 3=234 bytes 空间
- 接口名中的 `flash` 仅为参考，实际使用任何可掉电保存的介质皆可
- `is_legal_data` 用于判断 ft 数据是否合法，需用户自行实现

---

## 6. 主要数据结构

### 6.1 测距结果结构体

```c
typedef struct {
    dtof_uint16_t frame_id;        // 帧号
    dtof_int16_t first_target;     // 距离值，单位 mm
    dtof_uint16_t first_intensity; // 目标强度
    dtof_uint16_t main_nflash;     // 主闪光
    dtof_real32_t ambient;         // 环境光
    dtof_int32_t is_legal_frame;   // 是否合法帧, 1合法, 0非法

    // 以下为 debug 数据，可不关注
    dtof_real32_t main_peak_pos;
    dtof_real32_t second_peak_pos;
    dtof_real32_t ref_peak_pos;
    dtof_real32_t ref_peak_pos_smooth;
    dtof_uint16_t ref_peak_hist;
    dtof_real32_t first_target_raw;
    dtof_real32_t reflect_compensation;
    dtof_real32_t ambient_compensation;
    dtof_bool_t is_swap_peak;
} dtof_distance_result_t;
```

### 6.2 FT 数据结构体

```c
typedef struct {
    dtof_uint16_t cg_data[CROSS_TALK_OTP_NUM]; // xtalk 数据
    dtof_uint16_t distance_k;                  // 测距 k 值
    dtof_int16_t distance_b;                   // 测距 b 值
    dtof_uint16_t ref_spad;                    // ref spad 值
    dtof_uint16_t bin_offset;                  // bin offset 值
} dtof_ft_data_t;

typedef struct {
    dtof_uint16_t ft_calibration_type; //校准类型
    dtof_ft_data_t dtof_ft_data;
} dtof_ft_cali_param_t;
```

---

## 7. diag测试接口

`sdk\diag\dtof_diag.c` 文件中写了一些测试接口，测试底层接口实现的正确性，包括：

```c
DTOF_RET dtof_reg_burst_read(dtof_uint8_t reg_addr, dtof_uint16_t *reg_data_p, dtof_uint16_t len);
DTOF_RET dtof_reg_burst_write(dtof_uint8_t reg_addr, dtof_uint16_t *reg_data_p, dtof_uint16_t len);
DTOF_RET dtof_reg_burst_write_burn(dtof_uint8_t reg_addr, const dtof_uint16_t *reg_data_p, dtof_uint16_t len);
DTOF_RET dtof_get_ft_data_from_flash_multi_mode(dtof_uint16_t *ft_data, dtof_uint16_t len, dtof_run_mode_e run_mode, dtof_bool_t *is_legal_data);
DTOF_RET dtof_set_ft_data_to_flash_multi_mode(dtof_uint16_t *ft_data, dtof_uint16_t len, dtof_run_mode_e run_mode);
```

项目移植好后可调用 `DTOF_RET dtof_diag_test(void)` 进行基础测试，返回 0 为成功，同时会有 `diag test pass` 的 log 输出，如果测试未通过，可把错误 log 传回给我们的开发人员做分析

测试通过后可以把此文件从项目中移除

## 8. 初始化流程

### 8.1 初始化接口

```c
DTOF_RET dtof_sensor_init(void);
```

### 8.2 初始化说明

`dtof_sensor_init()` 用于完成以下工作：

- 芯片复位与基础探测
- 设备固件加载
- 运行环境初始化
- 默认模式准备
- FT 数据装载与必要同步

说明：

- `dtof_sensor_init()` 应在启动测距前调用
- SDK 内部会管理首次初始化状态，避免重复完整初始化

---

## 9. 测距接口

### 9.1 启动与停止测距

```c
DTOF_RET dtof_start_distance_measure(void); // 启动测距
DTOF_RET dtof_stop_distance_measure(void);  // 停止测距
```

### 9.2 获取测距结果

中断模式：

```c
DTOF_RET dtof_get_distance_result(dtof_distance_result_t *result_info);
```

轮询模式：

```c
DTOF_RET dtof_get_distance_result_polling(dtof_distance_result_t *result_info_p, dtof_bool_t *is_new_frame);
```

---

## 10. 典型使用示例

```c
int main(int argc, char *argv[]) {
    dtof_distance_result_t distance_result;
    dtof_bool_t is_new_frame;

    DTOF_CHECK_RET(dtof_sensor_init(), "dtof sensor init failed\n");
    DTOF_CHECK_RET(dtof_start_distance_measure(), "dtof start distance mode failed\n");

    while(1){
        dtof_get_distance_result_polling(&distance_result, &is_new_flag);

        if (is_new_flag == DTOF_TRUE)
        {
            printf("distance = %d\n",distance_result.first_target);
        }
    }
}
```

---

## 11. FT 校准接口

### 11.1 校准入口

```c
DTOF_RET dtof_do_ft_calibration_all_mode(dtof_uint16_t ft_cali_type, dtof_uint16_t ft_actual_param);
```

支持的校准类型包括：

- `DTOF_FT_CALIBRATE_BINOFFSET`
- `DTOF_FT_CALIBRATE_REFSPAD`
- `DTOF_FT_CALIBRATE_CG`
- `DTOF_FT_CALIBRATE_B`

校准数据中一般 xtalk 数据和 b 数据需要用户重新校准，其他使用出厂校准值

### 11.2 使用说明

#### 11.2.1 xtalk 校准

- xtalk 校准需要 ToF 芯片与校准面距离 = 600 mm
- 接口调用方法为
```c
dtof_do_ft_calibration_all_mode(DTOF_FT_CALIBRATE_CG, 1)
```

#### 11.2.1 b 校准

- 接口调用方法为
```c
dtof_do_ft_calibration_all_mode(DTOF_FT_CALIBRATE_B, distance)
```
- 参数中的 distance 为芯片与校准面距离，单位 mm

---

## 12. 版本查询接口

### sdk版本

```c
const char *dtof_get_sdk_version(void);
```

### lib版本

```c
const char *dtof_get_lib_version(void);
```

### 芯片内部软件版本

```c
dtof_uint16_t dtof_get_chip_version(void);
```
