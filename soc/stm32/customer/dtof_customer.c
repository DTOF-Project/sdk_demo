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
int dtof_reg_burst_write(uint8_t reg_addr, uint16_t *reg_data_p, uint16_t len)
{
    dtof_device_t *dev_p;
    Sensor_Status ret;

    if (!reg_data_p || len == 0U)
    {
        return DTOF_RET_INVALID_PARAM;
    }

    dev_p = ds_device_get();
    if (!dev_p)
    {
        return DTOF_RET_DEVICE_ERROR;
    }

    if (DTOF_BIT_CHECK(dev_p->sensor_flags, SENSOR_F_LITTLEENDIAN))
    {
        dtof_convert_endian(reg_data_p, len);
    }

    ret = Sensor_IIC_Write_X_Bytes(reg_addr, (uint8_t *)reg_data_p, len);

    if (DTOF_BIT_CHECK(dev_p->sensor_flags, SENSOR_F_LITTLEENDIAN))
    {
        dtof_convert_endian(reg_data_p, len);
    }

    return (ret == SENSOR_RET_SUCCESS) ? DTOF_RET_SUCCESS : DTOF_RET_DEVICE_ERROR;
}

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
int dtof_reg_burst_read(uint8_t reg_addr, uint16_t *reg_data_p, uint16_t len)
{
    if (!reg_data_p || len == 0U)
    {
        return DTOF_RET_INVALID_PARAM;
    }

    if (Sensor_IIC_Read_X_Bytes(reg_addr, (uint8_t *)reg_data_p, len) != SENSOR_RET_SUCCESS)
    {
        return DTOF_RET_DEVICE_ERROR;
    }

    return DTOF_RET_SUCCESS;
}

void dtof_set_interrupt_flag(dtof_bool_t flag)
{
    g_interrupt_flag = flag;
}

dtof_bool_t dtof_get_interrupt_flag(void)
{
    return g_interrupt_flag;
}

void dtof_sleep_ms(dtof_uint32_t time)
{
    usleep(time * 1000);
}

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
    dtof_uint16_t reg_data = 0U;
    dtof_uint8_t reg_addr;

    if (value == NULL)
    {
        return SENSOR_RET_FAILED;
    }

    reg_addr =(dtof_uint8_t)(addr >> 1);

    if (dtof_reg_burst_read(reg_addr,&reg_data,1U)!= DTOF_RET_SUCCESS)
    {
        return SENSOR_RET_FAILED;
    }

    if ((addr & 0x01U) == 0U)
    {
        *value =(dtof_uint8_t)(reg_data & 0x00FFU);
    }

    else
    {
        *value =(dtof_uint8_t)((reg_data >> 8)& 0x00FFU);
    }

    return SENSOR_RET_SUCCESS;
}

void Test_IIC_Read_Compare(uint8_t reg_addr)
{
    dtof_uint16_t word_data = 0U;
    uint8_t byte0 = 0U;
    uint8_t byte1 = 0U;

    if (dtof_reg_burst_read(reg_addr, &word_data, 1U) != DTOF_RET_SUCCESS)
    {
        dtof_printf("dtof_reg_burst_read FAIL\n");
        return;
    }

    if (Sensor_IIC_Read_One_Byte((uint8_t)(reg_addr * 2U), &byte0) != SENSOR_RET_SUCCESS)
    {
        dtof_printf("byte0 FAIL\n");
        return;
    }

    if (Sensor_IIC_Read_One_Byte((uint8_t)(reg_addr * 2U + 1U), &byte1) != SENSOR_RET_SUCCESS)
    {
        dtof_printf("byte1 FAIL\n");
        return;
    }

    dtof_printf("word  = 0x%04X\n", word_data);
    dtof_printf("byte0 = 0x%02X\n", byte0);
    dtof_printf("byte1 = 0x%02X\n", byte1);
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
    dtof_device_t *dev_p;

    // 参数检查
    if (!value || len == 0) {
        return DTOF_RET_INVALID_PARAM;
    }

    dev_p = ds_device_get();
    if (!dev_p) {
        return DTOF_RET_DEVICE_ERROR;
    }

    // 字节序转换
    if (DTOF_BIT_CHECK(dev_p->sensor_flags, SENSOR_F_LITTLEENDIAN)) {
        dtof_convert_endian(value, tlen);
    }

    // 写入数据
    ret = device_write_block(
        dev_p->dsd_peripheral.common_cfg.comm_channel_id,
        addr,
        (uint8_t*)value,
        tlen
    );

    // 恢复字节序
    if (DTOF_BIT_CHECK(dev_p->sensor_flags, SENSOR_F_LITTLEENDIAN)) {
        dtof_convert_endian(value, tlen);
    }

    return ret;
}

void Test_IIC_Read_X_Bytes(uint8_t addr, uint16_t tlen)
{
    uint8_t data[32];
    uint16_t i;

    if ((tlen == 0U) || (tlen > sizeof(data)))
    {
        dtof_printf("invalid len\n");
        return;
    }

    if (Sensor_IIC_Read_X_Bytes(addr, data, tlen) != SENSOR_RET_SUCCESS)
    {
        dtof_printf("Sensor_IIC_Read_X_Bytes: FAIL\n");
        return;
    }

    dtof_printf("Sensor_IIC_Read_X_Bytes: PASS\n");

    for (i = 0U; i < tlen; i++)
    {
        dtof_printf("addr=0x%02X data=0x%02X\n", (uint8_t)(addr + i), data[i]);
    }
}


Sensor_Status Sensor_IIC_Write_One_Byte(uint8_t addr,uint8_t value)
{
    dtof_uint16_t reg_data = 0U;

    dtof_uint8_t reg_addr;
    DTOF_RET ret;

    reg_addr =(dtof_uint8_t)(addr >> 1);


    if (dtof_reg_burst_read(reg_addr,&reg_data,1U)!= DTOF_RET_SUCCESS)
    {
        return SENSOR_RET_FAILED;
    }


    if ((addr & 0x01U) == 0U)
    {
        reg_data =(dtof_uint16_t)((reg_data & 0xFF00U) | value);
    }


    else
    {
        reg_data =(dtof_uint16_t)((reg_data & 0x00FFU) | ((dtof_uint16_t)value << 8));
    }

    ret = dtof_reg_burst_write(reg_addr, &reg_data, 1U);
    
    

    return (ret == DTOF_RET_SUCCESS)? SENSOR_RET_SUCCESS: SENSOR_RET_FAILED;
}

void Test_IIC_Write_One_Byte(uint8_t addr, uint8_t value)
{
    uint8_t read_value;
    uint16_t reg_before;
    uint16_t reg_after;
    uint8_t reg_addr = (uint8_t)(addr >> 1);
    
     dtof_set_mcu_status(DTOF_MCU_STATE_SLEEP_DIRECT);

    dtof_reg_burst_read(reg_addr, &reg_before, 1U);

    dtof_printf("before reg[0x%02X]=0x%04X\n", reg_addr, reg_before);

    if (Sensor_IIC_Write_One_Byte(addr, value) != SENSOR_RET_SUCCESS)
    {
        dtof_printf("Sensor_IIC_Write_One_Byte: FAIL\n");
        return;
    }

    dtof_reg_burst_read(reg_addr, &reg_after, 1U);

    dtof_printf("after  reg[0x%02X]=0x%04X\n", reg_addr, reg_after);

    if (Sensor_IIC_Read_One_Byte(addr, &read_value) != SENSOR_RET_SUCCESS)
    {
        dtof_printf("Sensor_IIC_Read_One_Byte: FAIL\n");
        return;
    }
    
     dtof_set_mcu_status(DTOF_MCU_STATE_WAKEUP);

    dtof_printf("write=0x%02X read=0x%02X\n", value, read_value);
}

Sensor_Status Sensor_IIC_Write_X_Bytes(uint8_t addr,uint8_t *pValue,uint16_t tlen)
{
    int ret;
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
    ret = device_read_block(
        dev_p->dsd_peripheral.common_cfg.comm_channel_id,
        addr,
        (uint8_t*)pValue,
        tlen
    );

    // 字节序转换
    if (ret == DTOF_RET_SUCCESS &&
        DTOF_BIT_CHECK(dev_p->sensor_flags, SENSOR_F_LITTLEENDIAN)) {
        dtof_convert_endian(pValue, tlen);
    }
    return ret;


    return SENSOR_RET_SUCCESS;
}

void Test_IIC_Write_X_Bytes(uint8_t addr, uint8_t *write_data, uint16_t tlen)
{
    
}

/* ============================================================================
 * 4.2 延时接口
 * ==========================================================================*/

/**
 * @brief 毫秒级延时
 */
void Sensor_Delay_Ms(uint16_t nMs)
{
    dtof_sleep_ms((dtof_uint32_t)nMs);
}
