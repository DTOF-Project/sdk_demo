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

extern void stm32_flash_write(uint32_t offset, uint64_t* context, uint16_t len);
extern void stm32_flash_read(uint32_t offset, uint64_t* context, uint16_t len);
extern void stm32_flash_write_u64(uint32_t offset, uint64_t *context, uint16_t num_words);
extern void stm32_flash_read_u64(uint32_t offset, uint64_t *context, uint16_t num_words);

DTOF_RET dtof_get_distance_offset_from_flash(dtof_uint8_t device_id, dtof_int32_t *distance_offset)
{
    uint64_t distance_offset_64;
    stm32_flash_read_u64(0, &distance_offset_64, 1);
    *distance_offset = (dtof_int32_t)distance_offset_64;
    printf("read distance offset: %d\n", *distance_offset);
    return DTOF_RET_SUCCESS;
}

DTOF_RET dtof_set_distance_offset_to_flash(dtof_uint8_t device_id, dtof_int32_t distance_offset)
{
    uint64_t distance_offset_64 = (uint64_t)distance_offset;
    stm32_flash_write_u64(0, (uint64_t*)&distance_offset_64, 1);
    printf("write distance offset: %d\n", (dtof_int32_t)distance_offset_64);
    return DTOF_RET_SUCCESS;
}

DTOF_RET dtof_get_xtalk_data_from_flash(dtof_uint8_t device_id, dtof_uint16_t *xtalk_data)
{
    uint64_t xtalk_data64[18];
    stm32_flash_read_u64(64, xtalk_data64, 18);

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
    stm32_flash_write_u64(64, xtalk_data64, 18);
    return DTOF_RET_SUCCESS;
}

