# DTOF RefSPAD 校准流程说明

## 1. 功能概述
RefSPAD校准用于选择合适的参考光路SPAD组合，确保参考光信号强度在合适范围内。

## 2. 校准流程

### 2.1 初始化准备
1. 关闭所有参考光路SPAD
2. 获取OTP中存储的SPAD掩码（otp_ref_spad_mask）

### 2.2 SPAD特性测量
1. 遍历每个SPAD（总共8个）：
   - 根据OTP掩码判断当前SPAD是否可用
   - 单独打开当前SPAD
   - 采集一帧数据
   - 读取MV1值（地址：0x40d）
   - 记录SPAD索引和对应的MV1值

### 2.3 数据处理
1. 对所有SPAD按MV1值进行排序（从小到大）
2. 计算最优SPAD组合：
   - 从MV1值最小的SPAD开始累加
   - 当累加值接近但不超过阈值（400）时停止
   - 生成最终的SPAD掩码

### 2.4 结果确认
1. 配置计算得到的SPAD掩码
2. 验证参考光信号强度

## 3. 关键参数
- `REF_SPAD_MAX_NUM`: 8（SPAD总数）
- `REF_SPAD_MV1_SUM_MAX`: 400（MV1累加值上限）
- `MV1_RAM_ADDR`: 0x40d（MV1值读取地址）

## 4. 数据结构
```c
typedef struct spade_info_ {
    dtof_uint16_t sapd_index;   // SPAD索引
    dtof_uint16_t spad_value;   // 对应的MV1值
} spade_info_t;
```

## 5. 注意事项
1. 校准前需确保OTP中的SPAD掩码有效
2. 单个SPAD测试时需要完全关闭其他SPAD
3. MV1值需要在合理范围内
4. 最终选择的SPAD组合需要满足总能量要求

## 6. 优化建议
1. 可以增加MV1值有效性判断
2. 考虑添加温度补偿机制
3. 可以增加多帧平均处理
4. 优化SPAD选择算法，考虑空间分布

## 7. 错误处理
1. 参数有效性检查
2. 硬件操作失败处理
3. 计算结果合理性验证
4. 完整的错误码返回机制