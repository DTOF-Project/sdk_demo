/**
 * @file dtof_customer.c
 * @brief DTOF客户接口实现
 * @author liuzihao
 * @date 2025/1/6
 */
#include "inc/dtof_base_type.h"
#include "inc/dtof_endian.h"
#include "inc/dtof_log.h"
#include "user/device/ds_sal.h"
#include "user/device/ds_dev.h"
#include "user/device/device.h"
#include "inc/dtof_common.h"
#include "dtof_customer.h"
#include "base/inc/mos_platform.h"
#include "platform_user_config.h"
#include "inc/dtof_api.h"
#include "inc/dtof_float.h"
#include "inc/lib/dtof_ft.h"
#include "inc/dtof_calibration_ft.h"
#include "inc/dtof_driver.h"

// 中断状态标志
volatile dtof_bool_t g_interrupt_flag = DTOF_FALSE;



// 字节序转换辅助函数
static void dtof_convert_endian(uint16_t *data, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++) {
        data[i] = DTOF_SWAP16(data[i]);
    }
}

#if 0
int dtof_reg_burst_write(uint8_t reg_addr, uint16_t *reg_data_p, uint16_t len)
{
    int ret;
    dtof_device_t *dev_p;

    // 参数检查
    if (!reg_data_p || len == 0) {
        return DTOF_RET_INVALID_PARAM;
    }

    dev_p = ds_device_get();
    if (!dev_p) {
        return DTOF_RET_DEVICE_ERROR;
    }

    // 字节序转换
    if (DTOF_BIT_CHECK(dev_p->sensor_flags, SENSOR_F_LITTLEENDIAN)) {
        dtof_convert_endian(reg_data_p, len);
    }

    // 写入数据
    ret = device_write_block(
        dev_p->dsd_peripheral.common_cfg.comm_channel_id,
        reg_addr,
        (uint8_t*)reg_data_p,
        len
    );

    // 恢复字节序
    if (DTOF_BIT_CHECK(dev_p->sensor_flags, SENSOR_F_LITTLEENDIAN)) {
        dtof_convert_endian(reg_data_p, len);
    }

    return ret;
}
#endif

int dtof_reg_burst_write_burn(uint8_t reg_addr, const uint16_t *reg_data_p, uint16_t len)
{
    int ret;
    dtof_device_t *dev_p;

    // 参数检查
    if (!reg_data_p || len == 0) {
        return DTOF_RET_INVALID_PARAM;
    }

    dev_p = ds_device_get();
    if (!dev_p) {
        return DTOF_RET_DEVICE_ERROR;
    }

    // 写入数据
    ret = device_write_block(
        dev_p->dsd_peripheral.common_cfg.comm_channel_id,
        reg_addr,
        (uint8_t*)reg_data_p,
        len
    );

    return ret;
}

#if 0
int dtof_reg_burst_read(uint8_t reg_addr, uint16_t *reg_data_p, uint16_t len)
{
    int ret;
    dtof_device_t *dev_p;

    // 参数检查
    if (!reg_data_p || len == 0) {
        return DTOF_RET_INVALID_PARAM;
    }

    dev_p = ds_device_get();
    if (!dev_p) {
        return DTOF_RET_DEVICE_ERROR;
    }

    // 读取数据
    ret = device_read_block(
        dev_p->dsd_peripheral.common_cfg.comm_channel_id,
        reg_addr,
        (uint8_t*)reg_data_p,
        len
    );

    // 字节序转换
    if (ret == DTOF_RET_SUCCESS &&
        DTOF_BIT_CHECK(dev_p->sensor_flags, SENSOR_F_LITTLEENDIAN)) {
        dtof_convert_endian(reg_data_p, len);
    }
    return ret;
}
#endif

void dtof_set_interrupt_flag(dtof_bool_t flag)
{
    g_interrupt_flag = flag;
}

dtof_bool_t dtof_get_interrupt_flag(void)
{
    return g_interrupt_flag;
}

// void dtof_sleep_ms(dtof_uint32_t time)
// {
//     usleep(time * 1000);
// }

void stm32_flash_write_init(uint32_t page, uint32_t page_num)
{
    FLASH_EraseInitTypeDef eraseInit;
    uint32_t pageError = 0;

    // 解锁Flash
    if (HAL_FLASH_Unlock() != HAL_OK)
    {
        printf("flash unlock failed\n");
        return;
    }
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_OPTVERR);
    eraseInit.TypeErase = FLASH_TYPEERASE_PAGES;
    eraseInit.NbPages = page_num; // 向上取整
    eraseInit.Page = page;
    eraseInit.Banks = FLASH_BANK_2;

    do
    {
    } while (HAL_FLASHEx_Erase(&eraseInit, &pageError) != HAL_OK);
}

void stm32_flash_write_u64(uint32_t offset, uint64_t *context, uint16_t num_words)
{
    uint32_t flash_addr = offset;

    HAL_FLASH_Unlock();
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_OPTVERR);

    for (uint32_t i = 0; i < num_words; i++) {
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, flash_addr + i * 8, context[i]) != HAL_OK) {
            HAL_FLASH_Lock();
            return;
        }
    }

    HAL_FLASH_Lock();
}

void stm32_flash_read_u64(uint32_t offset, uint64_t *context, uint16_t num_words)
{
    uint32_t flash_addr = offset;
    for (uint32_t i = 0; i < num_words; i++) {
        context[i] = *(uint64_t *)(flash_addr + i * 8);
    }
}

#define FT_DATA_NUM (sizeof(dtof_ft_cali_param_t)/sizeof(dtof_uint16_t))
DTOF_RET dtof_get_ft_data_from_flash(dtof_uint16_t *ft_data, dtof_uint16_t len, dtof_bool_t *is_legal_data)
{
    uint64_t ft_data64[FT_DATA_NUM];
    stm32_flash_read_u64(DTOF_FT_DATA_FLASH_PAGE_START_ADDR, ft_data64, FT_DATA_NUM);

    *is_legal_data = DTOF_FALSE;
    for(dtof_uint16_t i = 0; i < FT_DATA_NUM; i++)
    {
        if (ft_data64[i] != 0xFFFFFFFFFFFFFFFF)
        {
            *is_legal_data = DTOF_TRUE;
            break;
        }
    }

    for(dtof_uint16_t i = 0; i < FT_DATA_NUM; i++)
    {
        *(ft_data+i) = (dtof_uint16_t)ft_data64[i];
    }

    return DTOF_RET_SUCCESS;
}

DTOF_RET dtof_set_ft_data_to_flash(dtof_uint16_t *ft_data, dtof_uint16_t len)
{
    uint64_t ft_data64[FT_DATA_NUM];

    for(dtof_uint16_t i = 0; i < FT_DATA_NUM; i++)
    {
        ft_data64[i] = (uint64_t)(*(ft_data + i));
    }

    stm32_flash_write_init(DTOF_FT_DATA_FLASH_PAGE, DTOF_FT_DATA_FLASH_PAGE_NUM);
    stm32_flash_write_u64(DTOF_FT_DATA_FLASH_PAGE_START_ADDR, ft_data64, FT_DATA_NUM);

    return DTOF_RET_SUCCESS;
}

DTOF_RET dtof_get_ft_data_from_flash_multi_mode(dtof_uint16_t *ft_data, dtof_uint16_t len, dtof_run_mode_e run_mode, dtof_bool_t *is_legal_data)
{
    uint64_t ft_data64[FT_DATA_NUM];
    stm32_flash_read_u64((0x08000000 + (DTOF_FT_DATA_FLASH_PAGE + run_mode) * FLASH_PAGE_SIZE), ft_data64, FT_DATA_NUM);

    *is_legal_data = DTOF_FALSE;
    for(dtof_uint16_t i = 0; i < FT_DATA_NUM; i++)
    {
        if (ft_data64[i] != 0xFFFFFFFFFFFFFFFF)
        {
            *is_legal_data = DTOF_TRUE;
            break;
        }
    }

    for(dtof_uint16_t i = 0; i < FT_DATA_NUM; i++)
    {
        *(ft_data+i) = (dtof_uint16_t)ft_data64[i];
    }

    return DTOF_RET_SUCCESS;
}

DTOF_RET dtof_set_ft_data_to_flash_multi_mode(dtof_uint16_t *ft_data, dtof_uint16_t len, dtof_run_mode_e run_mode)
{
    uint64_t ft_data64[FT_DATA_NUM];

    for(dtof_uint16_t i = 0; i < FT_DATA_NUM; i++)
    {
        ft_data64[i] = (uint64_t)(*(ft_data + i));
    }

    stm32_flash_write_init(DTOF_FT_DATA_FLASH_PAGE + run_mode, DTOF_FT_DATA_FLASH_PAGE_NUM);
    stm32_flash_write_u64((0x08000000 + (DTOF_FT_DATA_FLASH_PAGE + run_mode) * FLASH_PAGE_SIZE), ft_data64, FT_DATA_NUM);

    return DTOF_RET_SUCCESS;
}

DTOF_RET dtof_get_distance_result_no_swap(dtof_distance_result_t *result_info_p)
{
    DTOF_CHECK_PTR(result_info_p);

    dtof_uint16_t distance_result[DTOF_DISTANCE_RESULT_LEN];

    DTOF_CHECK_RET(dtof_reg_burst_read(DTOF_SAVE_RESULT_REG_ADDR, distance_result, DTOF_DISTANCE_RESULT_LEN), "读取距离结果失败");

    // 填充结果数据
    result_info_p->frame_id = distance_result[DTOF_FIFO_FRAME_ID];
    result_info_p->first_target = (dtof_int16_t)(distance_result[DTOF_FIFO_DISTANCE0] / DTOF_DISTANCE_RESULT_DIVISOR - DTOF_DISTANCE_RESULT_OFFSET);
    result_info_p->first_intensity = distance_result[DTOF_FIFO_INTENSITY0] >> DTOF_DISTANCE_INTENSITY_SHIFT;
    result_info_p->main_nflash = distance_result[DTOF_FIFO_MAIN_NFLASH];

    // 计算主噪声
    result_info_p->ambient = dtof_div((distance_result[DTOF_FIFO_NOISE_LOW] + ((distance_result[DTOF_FIFO_NOISE_HIGH] & 0x3c) << 14)), (dtof_real32_t)DTOF_MAIN_NOISE_DIV);

    // 判断是否合法帧
    result_info_p->is_legal_frame = dtof_clac_confidence(result_info_p->first_target, result_info_p->first_intensity, result_info_p->ambient);

    if (result_info_p->first_target < 0)
    {
        result_info_p->first_target = DTOF_MINIMUM_DISTANCE;
    }

    return DTOF_RET_SUCCESS;
}


Sensor_Status Sensor_IIC_Read_One_Byte(uint8_t addr,uint8_t *value)
{
    int ret;
    int sensor_state;
    dtof_device_t *dev_p;

    // 参数检查
    if (!value) {
        return DTOF_RET_INVALID_PARAM;
    }

    dev_p = ds_device_get();
    if (!dev_p) {
        return DTOF_RET_DEVICE_ERROR;
    }

    // 读取数据
    ret = device_read_block(
        dev_p->dsd_peripheral.common_cfg.comm_channel_id,
        addr,
        value,
        1U
    );

    // 字节序转换
    if (ret == DTOF_RET_SUCCESS &&
        DTOF_BIT_CHECK(dev_p->sensor_flags, SENSOR_F_LITTLEENDIAN)) {
        dtof_convert_endian((dtof_uint16_t *)value, 1U);
        sensor_state = SENSOR_RET_SUCCESS;
    }
    else
    {
        sensor_state = SENSOR_RET_FAILED;
    }

    return sensor_state;
}


/**
 * @brief I2C 读取多个字节
 *
 * 当前为了 Wrapper 简洁，
 * 直接复用单字节读取。
 *
 * 自动处理奇偶地址。
 */
Sensor_Status Sensor_IIC_Read_X_Bytes(uint8_t addr,uint8_t *value,uint16_t tlen)
{
    int ret;
    int sensor_state;
    dtof_device_t *dev_p;

    // 参数检查
    if (!value || tlen == 0) {
        return DTOF_RET_INVALID_PARAM;
    }

    dev_p = ds_device_get();
    if (!dev_p) {
        return DTOF_RET_DEVICE_ERROR;
    }

    // 读取数据
    ret = device_read_block(
        dev_p->dsd_peripheral.common_cfg.comm_channel_id,
        addr,
        value,
        tlen
    );

    // 字节序转换
    if (ret == DTOF_RET_SUCCESS &&
        DTOF_BIT_CHECK(dev_p->sensor_flags, SENSOR_F_LITTLEENDIAN)) {
        dtof_convert_endian((dtof_uint16_t *)value, tlen);
        sensor_state = SENSOR_RET_SUCCESS;
    }
    else{
        sensor_state = SENSOR_RET_FAILED;
    }

    return sensor_state;
}

Sensor_Status Sensor_IIC_Write_One_Byte(uint8_t addr,uint8_t *value)
{
    int ret;
    int sensor_state;
    dtof_device_t *dev_p;

    // 参数检查
    if (!value) {
        return DTOF_RET_INVALID_PARAM;
    }

    dev_p = ds_device_get();
    if (!dev_p) {
        return DTOF_RET_DEVICE_ERROR;
    }

    // 读取数据
    ret = device_write_block(
        dev_p->dsd_peripheral.common_cfg.comm_channel_id,
        addr,
        value,
        1U
    );

    // 字节序转换
    if (ret == DTOF_RET_SUCCESS &&
        DTOF_BIT_CHECK(dev_p->sensor_flags, SENSOR_F_LITTLEENDIAN)) {
        dtof_convert_endian((dtof_uint16_t *)value, 1U);
        sensor_state = SENSOR_RET_SUCCESS;
    }
    else{

        sensor_state = SENSOR_RET_FAILED;
    }

    return sensor_state;
}

Sensor_Status Sensor_IIC_Write_X_Bytes(uint8_t addr,uint8_t *pValue,uint16_t tlen)
{
    int ret;
    int sensor_state;
    dtof_device_t *dev_p;

    // 参数检查
    if (!pValue || tlen == 0) {
        return DTOF_RET_INVALID_PARAM;
    }

    dev_p = ds_device_get();
    if (!dev_p) {
        return DTOF_RET_DEVICE_ERROR;
    }

    // 读取数据
    ret = device_write_block(
        dev_p->dsd_peripheral.common_cfg.comm_channel_id,
        addr,
        pValue,
        tlen
    );

    // 字节序转换
    if (ret == DTOF_RET_SUCCESS &&
        DTOF_BIT_CHECK(dev_p->sensor_flags, SENSOR_F_LITTLEENDIAN)) {
        dtof_convert_endian((dtof_uint16_t *)pValue, tlen);
        sensor_state = SENSOR_RET_SUCCESS;
    }
    else{

        return SENSOR_RET_FAILED;
    }

    return ret;
}

/**
 * @brief 毫秒级延时
 */
void Sensor_Delay_Ms(uint16_t nMs)
{
    usleep(nMs * 1000);
}
