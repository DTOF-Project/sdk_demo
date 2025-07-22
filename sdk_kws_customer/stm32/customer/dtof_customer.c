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

// 中断状态标志
volatile dtof_bool_t g_interrupt_flag = DTOF_FALSE;

// 字节序转换辅助函数
static void dtof_convert_endian(uint16_t *data, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++) {
        data[i] = DTOF_SWAP16(data[i]);
    }
}

int dtof_reg_burst_write(uint8_t device_id, uint8_t reg_addr, uint16_t *reg_data_p, uint16_t len)
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

int dtof_reg_burst_write_burn(uint8_t device_id, uint8_t reg_addr, const uint16_t *reg_data_p, uint16_t len)
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


int dtof_reg_burst_read(uint8_t device_id, uint8_t reg_addr, uint16_t *reg_data_p, uint16_t len)
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

static ds_sal_config_t peripheral;

static int board_peripheral()
{
    peripheral.common_cfg.comm_type = COMM_IIC;
    peripheral.common_cfg.comm_channel_id = IIC_NBR0;

    peripheral.pin_cfg[dssal_intr_pin].pin_id = PLATFORM_INTERRUPT_PIN;
    peripheral.pin_cfg[dssal_intr_pin].pin_config = PLATFORM_INTERRUPT_CFG;
    peripheral.pin_set[dssal_intr_pin].pin_default_value = MOS_GPIO_PULL_DOWN;

    peripheral.pin_cfg[dssal_reset_pin].pin_id = PLATFORM_RST_PIN;
    peripheral.pin_cfg[dssal_reset_pin].pin_config = PLATFORM_RST_CFG;
    peripheral.pin_set[dssal_reset_pin].pin_default_value = MOS_GPIO_PULL_DOWN;

    peripheral.pin_cfg[dssal_vcc_en_pin].pin_id = PLATFORM_VCC_PIN;
    peripheral.pin_cfg[dssal_vcc_en_pin].pin_config = PLATFORM_VCC_CFG;
    peripheral.pin_set[dssal_vcc_en_pin].pin_default_value = MOS_GPIO_PULL_UP;

    peripheral.pin_cfg[dssal_vcc1_en_pin].pin_id = PLATFORM_VCC1_PIN;
    peripheral.pin_cfg[dssal_vcc1_en_pin].pin_config = PLATFORM_VCC1_CFG;
    peripheral.pin_set[dssal_vcc1_en_pin].pin_default_value = MOS_GPIO_PULL_UP;

    peripheral.pin_cfg[dssal_1v2_en_pin].pin_id = PLATFORM_VCC1V2_PIN;
    peripheral.pin_cfg[dssal_1v2_en_pin].pin_config = PLATFORM_VCC1V2_CFG;
    peripheral.pin_set[dssal_1v2_en_pin].pin_default_value = MOS_GPIO_PULL_UP;

    peripheral.pin_cfg[dssal_1v8_en_pin].pin_id = PLATFORM_VCC1V8_PIN;
    peripheral.pin_cfg[dssal_1v8_en_pin].pin_config = PLATFORM_VCC1V8_CFG;
    peripheral.pin_set[dssal_1v8_en_pin].pin_default_value = MOS_GPIO_PULL_UP;

    return 0;
}

DTOF_RET dtof_peripheral_device_init(void)
{
    /*
     * 1. 初始化gpio
     *  1.1 interrupt pin: (STM32_PIN_MODE_IT_FALLING | STM32_PIN_PULL_PULLUP | STM32_PIN_SPEED_HIGH)
     *  1.2 reset pin: (STM32_PIN_MODE_OUTPUT_PP | STM32_PIN_PULL_NOPULL | STM32_PIN_SPEED_LOW) 默认拉低
     *  1.3 iic pin: (STM32_PIN_MODE_AF_OD | STM32_PIN_PULL_NOPULL | STM32_PIN_SPEED_VERY_HIGH | STM32_PIN_ALT_4)
     * 2. 初始化sensor
     *  2.1 reset pin init 后 usleep(100), 然后拉高reset pin, usleep(750)
     *  2.2 iic初始化
     *  2.3 注册中断
    */
    DTOF_RET ret;

    board_peripheral();

    DTOF_CHECK_RET(ds_device_peripheral_init(&peripheral), "peripheral init fail");

    return DTOF_RET_SUCCESS;
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


void stm32_flash_write(uint32_t offset, uint64_t *context, uint16_t len)
{
    // 计算目标Flash地址
    uint32_t flash_addr = offset;
    uint32_t context_index = 0;

    do
    {

    } while (HAL_FLASH_Unlock() != HAL_OK);
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_OPTVERR);

    // 按8字节写入数据
    for (uint32_t i = 0; i < len; i += 4)
    {
        uint64_t data = *(context + context_index);
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, flash_addr + (context_index * 8), data) != HAL_OK)
        {
            HAL_FLASH_Lock();
            return;
        }
        context_index++;
    }

    HAL_FLASH_Lock();
}

void stm32_flash_read(uint32_t offset, uint64_t *context, uint16_t len)
{
    // 计算目标Flash地址
    // len 的单位是2byte
    // 起始地址跳过64的预留byte
    uint32_t flash_addr = offset;
    uint32_t context_index = 0;
    uint64_t data = 0;

    // 按8字节写入数据
    for (uint32_t i = 0; i < len; i += 4)
    {
        data = *(uint64_t *)(flash_addr + context_index * 8);
        *(context + context_index) = data;
        context_index++;
    }
}

DTOF_RET dtof_get_distance_offset_from_flash(dtof_uint8_t device_id, dtof_int32_t *distance_offset)
{
    uint64_t distance_offset_64;
    stm32_flash_read_u64(DTOF_B_DATA_FLASH_PAGE_START_ADDR, &distance_offset_64, 1);
    *distance_offset = (dtof_int32_t)distance_offset_64;
    printf("read distance offset: %d\n", *distance_offset);
    return DTOF_RET_SUCCESS;
}

DTOF_RET dtof_set_distance_offset_to_flash(dtof_uint8_t device_id, dtof_int32_t distance_offset)
{
    uint64_t distance_offset_64 = (uint64_t)distance_offset;
    stm32_flash_write_u64(DTOF_B_DATA_FLASH_PAGE_START_ADDR, (uint64_t*)&distance_offset_64, 1);
    printf("write distance offset: %d\n", (dtof_int32_t)distance_offset_64);
    return DTOF_RET_SUCCESS;
}

DTOF_RET dtof_get_xtalk_data_from_flash(dtof_uint8_t device_id, dtof_uint16_t *xtalk_data)
{
    uint64_t xtalk_data64[18];
    stm32_flash_read_u64(DTOF_CG_DATA_FLASH_PAGE_START_ADDR, xtalk_data64, 18);

    printf("read xtalk data: ");
    for(dtof_uint16_t i = 0; i < 18; i++)
    {
        *(xtalk_data+i) = (dtof_uint16_t)xtalk_data64[i];
        printf("%d, ", xtalk_data[i]);
    }
    printf("\n");
    return DTOF_RET_SUCCESS;
}

DTOF_RET dtof_set_xtalk_data_from_flash(dtof_uint8_t device_id, dtof_uint16_t *xtalk_data)
{
    uint64_t xtalk_data64[18];
    for(dtof_uint16_t i = 0; i < 18; i++)
    {
        xtalk_data64[i] = (uint64_t)(*(xtalk_data + i));
    }
    stm32_flash_write_u64(DTOF_CG_DATA_FLASH_PAGE_START_ADDR, xtalk_data64, 18);
    return DTOF_RET_SUCCESS;
}

