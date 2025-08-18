SDK 移植与使用指南

1. 概述
本文档提供了在 SoC_app 环境下移植和使用 DTOF SDK 的详细步骤。该 SDK 用于控制和获取 ToF 传感器的测距数据。

2. 开发环境准备
2.1 硬件要求
ToF 传感器模块
目标微控制器开发板（如 STM32）
调试器（如 ST-Link）

2.2 软件要求
集成开发环境（如 Keil MDK、IAR Workbench 或 STM32CubeIDE）
SDK 源代码包

2.3 SDK 文件结构
SDK 主要包含以下文件：
CopyInsert
SoC_app/
├── sdk/
│   ├── src/
│   │   ├── dtof_api.c          # 主要 API 接口
│   │   ├── dtof_driver.c       # 底层驱动接口
│   │   ├── dtof_endian.c       # 字节序处理
│   │   └── dtof_fix16_float.c  # 定点数处理
│   ├── inc/                    # 头文件目录

3. 移植步骤
3.1 添加 SDK 源文件到项目
将以下核心文件添加到您的项目中：

- dtof_api.c
- dtof_driver.c
- dtof_endian.c
- dtof_fix16_float.c

添加 SDK 头文件路径到项目包含路径中：

- SoC_app/sdk/inc

3.2 添加lib文件到项目

3.2.1 使用iar

3.2.1.1 导入lib
3.2.1.1.1 右键项目->Options->Linker->Library
3.2.1.1.2 导入dtof_lib.a
3.2.1.1.3 确认sdk中的lib目录在项目的include路径中
3.2.1.1.4 尝试编译

3.2.1.2 使用方法
3.2.1.2.1 dtof_distance_result_t结构体中新增了dtof_int32_t is_legal_frame; 成员
3.2.1.2.2 is_legal_frame 含义是当前帧距离是否是合法距离, 1合法, 0非法

3.2.2 使用keil

4. 实现硬件抽象层接口
在 `dtof_driver.c` 中，您需要实现以下函数来适配您的硬件平台：

```c
// 批量读寄存器
DTOF_RET dtof_reg_burst_read(dtof_uint8_t device_id, dtof_uint8_t reg_addr, dtof_uint16_t *reg_data_p, dtof_uint16_t len);

// 批量写寄存器
DTOF_RET dtof_reg_burst_write(dtof_uint8_t device_id, dtof_uint8_t reg_addr, dtof_uint16_t *reg_data_p, dtof_uint16_t len);

// 毫秒延时函数
void dtof_sleep_ms(dtof_uint32_t time);

// 从flash读distance_offset
DTOF_RET dtof_get_distance_offset_from_flash(dtof_uint8_t device_id, dtof_int32_t *distance_offset);

// 把distance_offset写入flash
DTOF_RET dtof_set_distance_offset_to_flash(dtof_uint8_t device_id, dtof_int32_t distance_offset);

// 从flash读xtalk_data
DTOF_RET dtof_get_xtalk_data_from_flash(dtof_uint8_t device_id, dtof_uint16_t *xtalk_data);

// 把xtalk_data写入flash
DTOF_RET dtof_set_xtalk_data_from_flash(dtof_uint8_t device_id, dtof_uint16_t *xtalk_data)
```

注: 读写flash的接口目前针对寄过来的15颗模组做了简单实现, 如果想直接测试, 不重新校准, 可以暂时不修改以上四个读写flash的函数

4.1 I2C/SPI 通信接口实现示例 (IIC地址为0x41) (以 STM32 为例)

```c
#define DEVICE_I2C_ADDRESS  0X41
#define DEVICE_MAX_NUM 2
extern I2cDef_s I2c_def_list[DEVICE_MAX_NUM];
void I2c_ReadData(I2cDef_s I2c_def, uint8_t device_address, uint8_t reg_addr, uint8_t* dat, uint16_t num){
    I2C_start(I2c_ef);
    I2C_SendByte(device_address|Write, I2c_def);
    I2C_sendByte(reg_addr,I2c_ef);
    I2C_start(I2c_def);
    I2C_SendByte(device_address|Read,I2c_def);
    for(uint16_t i=0;i<num-l;i++){
        *(dat+i)=I2C_ReadByte(1，I2c_def);
    }
    *(dat+i)=I2C_ReadByte(0，I2c_def);
    I2C_stop(I2c_def);
}

DTOF_RET dtof_reg_burst_read(dtof_uint8_t device_id, dtof_uint8_t reg_addr, dtof_uint16_t *reg_data_p, dtof_uint16_t len)
{
    if(device_id > DEVICE_MAX_NUM){
        return DTOF_ERROR;
    }
    // u16 to u8
    dtof_uint16_t num = len*2;
    I2cDef_s I2c_def = I2c_def_list[device_id];
    I2c_ReadData(I2c_def, DEVICE_I2C_ADDRESS, reg_addr, (uint8_t*)reg_data_p, num);
    dtof_convert_endian(reg_data_p, len);
    return DTOF_OK;
}

DTOF_RET dtof_reg_burst_write(dtof_uint8_t device_id, dtof_uint8_t reg_addr, dtof_uint16_t *reg_data_p, dtof_uint16_t len)
{

    dtof_uint16_t num = len*2；
    dtof_convert_endian(reg_data_p,len);
    if(device_id>DEVICE_MAX_NUM)
        return DTOF_ERROR;
    if(device_id==0)
        I2cWriteBytes(DEVICE_I2C_ADDRESS,reg_addr,(uint8 t*)reg_data_p,num,Front_GP2AP_IIC);//左前
    else
        I2CWriteBytes(DEVICE_I2C_ADDRESS,reg addr,(uint8 t*)reg data p,num,Behind_GP2AP_IIC);//.前
    //调用后需要做一次字转换，做恢复功能
     dtof_convert_endian(reg_data_p,len);
    return DTOF RET SUCCESS;
}
```

4.2 延时函数实现示例

```c
void dtof_sleep_ms(dtof_uint32_t time)
{
    HAL_Delay(time);
}
```

5. SDK 使用流程

5.1 初始化 sensor

```c
    // init peripheral and wait inner mcu ready (uart, iic/spi)
    dtof_init_and_wait_for_ready(device_id, &chip_id, NORMAL_DISTANCE_MODE);

```

5.2 启动/停止测距

```c
// 启动连续测距模式
dtof_start_distance_measure(device_id);

// 停止测距
dtof_stop_distance_measure(device_id);
```

5.3 获取测距结果

```c
void main() {
    dtof_distance_result_t distance_result_front;
    dtof_distance_result_t distance_result_blind;
    dtof_uint8_t is_new_flag;
    dtof_init_and_wait_for_ready(0,&chip_id,NORMAL_DISTANCE_MODE);
    dtof_init_and_wait_for_ready(1,&chip id,NORMAL_DISTANCE_MODE);
    dtof_start_distance_measure(0);
    dtof_start_distance_measure(1);
    while(1){
        fwdgt_counterreload()
        PrintfTask();
        do {
            dtof_get_distance_result(0, NORMAL_DISTANCE_MODE, &distance_result_front, &is_new_flag)
        } while(is_new_flag == DTOF_FALSE)
        do {
            dtof_get_distance_result(0, NORMAL_DISTANCE_MODE, &distance_result_front, &is_new_flag)
        } while(is_new_flag == DTOF_FALSE)
        printf("distance result front===d\rn",distance_result_front.first_target);
        printf("distance result blind===gd\r\n",distance_result_blind.first_target);
    }
}
```

5.4 重新校准

5.4.1 xtalk校准
5.4.1.1 移动sensor到距离物体600mm处
5.4.1.2 注册步骤4中读写flash的接口
DTOF_RET dtof_get_xtalk_data_from_flash(dtof_uint8_t device_id, dtof_uint16_t *xtalk_data);
DTOF_RET dtof_set_xtalk_data_from_flash(dtof_uint8_t device_id, dtof_uint16_t *xtalk_data);
5.4.1.3 调用校准api
dtof_init_and_wait_for_ready(device_id, &chip_id, DO_XTALK_CALIBRATION_MODE);

5.4.2 距离校准
5.4.2.1 移动sensor到距离物体20mm处
5.4.2.2 注册步骤4中读写flash的接口
DTOF_RET dtof_get_distance_offset_from_flash(dtof_uint8_t device_id, dtof_int32_t *distance_offset);
DTOF_RET dtof_set_distance_offset_to_flash(dtof_uint8_t device_id, dtof_int32_t distance_offset);
5.4.2.3 调用校准api
dtof_init_and_wait_for_ready(device_id, &chip_id, DO_OFFSET_CALIBRATION_MODE);

注: 每次校准之后需要重新上下电

6. 故障排除
6.1 通信问题
检查 I2C/SPI 连接是否正确
验证通信时序是否符合要求
检查电源是否稳定

7.2 测距异常
确保传感器前方无遮挡
检查环境光干扰情况
验证校准参数是否正确

8. 参考资源
数据手册
SDK API 参考文档
示例代码（位于 SoC_app/app 目录）
9. 附录
9.1 RAM Code 说明
内部 MCU 实现了核心测距算法。在使用 SDK 时，您无需直接修改此文件，SDK 会通过注册的接口函数与内部 MCU 通信。

9.2 常见参数配置

如有任何问题或需要进一步的帮助，请参考完整的 SDK 文档或联系技术支持团队
