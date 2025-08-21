#include <time.h>

#include "i2c_init.h"
#include "dtof_customer.h"
#include "sdk/inc/dtof_base_type.h"
#include "sdk/inc/dtof_endian.h"
#include "sdk/inc/dtof_log.h"

extern device_driver_ops_t device_iic_driver_ops;

// #define DTOF_CUSTOMER_DEBUG_FLAG

DTOF_RET dtof_reg_burst_write(uint8_t device_id, uint8_t reg_addr, uint16_t *reg_data_p, uint16_t len) {

#ifdef DTOF_CUSTOMER_DEBUG_FLAG
    DTOF_LOG("write 0x%02x reg_data_p[0] = 0x%04x\n", reg_addr, reg_data_p[0]);
#endif

    // 主机uint16 -> 从机大端序uint8
    uint8_t *input_data_p = reg_data_p;
    DTOF_HOST_TO_BE16(input_data_p, reg_data_p);
    // DTOF_HOST_TO_LE16(input_data_p, reg_data_p);

#ifdef DTOF_CUSTOMER_DEBUG_FLAG
    // 大端序
    DTOF_LOG("write 0x%02x input_data_p[0], [1] = 0x%02x, 0x%02x\n", reg_addr, input_data_p[0], input_data_p[1]);
    // 小端序
    // DTOF_LOG("write 0x%02x input_data_p[1], [0] = 0x%02x, 0x%02x\n", reg_addr, input_data_p[1], input_data_p[0]);
#endif

    // 调用底层写函数
    int ret = device_iic_driver_ops.write_block(device_id, reg_addr, input_data_p, len);

    if (ret == DTOF_RET_ERROR) {
        perror("dtof_reg_burst_write() WRITE ERROR");
    }
    return ret;
}

DTOF_RET dtof_reg_burst_write_burn(uint8_t device_id, uint8_t reg_addr, const uint16_t *reg_data_p, uint16_t len) {
    // write 和 write_burn 的区别: 不需要再次大小端转换，烧写处理好（包括大小端转换）的程序

    // 调用底层写函数
    int ret = device_iic_driver_ops.write_block(device_id, reg_addr, reg_data_p, len);

    if (ret == DTOF_RET_ERROR) {
        perror("dtof_reg_burst_write_burn() WRITE ERROR");
    }
    return ret;
}

DTOF_RET dtof_reg_burst_read(uint8_t device_id, uint8_t reg_addr, uint16_t *reg_data_p, uint16_t len) {
    int ret = device_iic_driver_ops.read_block(device_id, reg_addr, (uint8_t *)reg_data_p, len);

// #ifdef DTOF_CUSTOMER_DEBUG_FLAG
//     DTOF_LOG("read 0x%02x reg_data_p[0] (src) = 0x%04x\n", reg_addr, reg_data_p[0]);
// #endif

    // 从机大端序uint8 -> 主机uint16
    for (uint16_t i = 0; i < len; i++) {
        reg_data_p[i] = DTOF_SWAP16(reg_data_p[i]);
        // reg_data_p[i] = DTOF_BE16_TO_HOST(reg_data_p[i]);
        // reg_data_p[i] = DTOF_LE16_TO_HOST(reg_data_p[i]);
    }

#ifdef DTOF_CUSTOMER_DEBUG_FLAG
    DTOF_LOG("read 0x%02x reg_data_p[0] (swaped) = 0x%04x\n", reg_addr, reg_data_p[0]);
#endif

    if (ret == DTOF_RET_ERROR) {
        perror("dtof_reg_burst_read() READ ERROR");
    }
    return ret;
}

// 中断状态标志
volatile dtof_bool_t g_interrupt_flag = DTOF_FALSE;

void dtof_set_interrupt_flag(dtof_bool_t flag) {
    g_interrupt_flag = flag;
}

dtof_bool_t dtof_get_interrupt_flag(void) {
    return g_interrupt_flag;
}

// sleep，时间单位ms
void dtof_sleep_ms(dtof_uint32_t time) {
    struct timespec ts;
    ts.tv_sec = time / 1000;
    ts.tv_nsec = (time % 1000) * 1000000; // 1毫秒 = 1,000,000纳秒
    
    // 使用nanosleep而不是usleep（因为usleep已被POSIX废弃）
    nanosleep(&ts, NULL);
}

DTOF_RET dtof_get_distance_offset_from_flash(dtof_uint8_t device_id, dtof_int32_t *distance_offset)
{
    *distance_offset = 0;

    return DTOF_RET_SUCCESS;
}

DTOF_RET dtof_get_xtalk_data_from_flash(dtof_uint8_t device_id, dtof_uint16_t *xtalk_data)
{
    // xtalk_data是uint16_t类型数组指针, 大小为18, 这里给默认值
    // dtof_uint16_t xtalk_data_default[] = {11, 258, 257, 514, 514, 514, 1284, 1285, 772, 1028, 771, 771, 514, 514, 514, 514, 258, 1};
    // dtof_memcpy(xtalk_data, xtalk_data_default, sizeof(xtalk_data_default));

    return DTOF_RET_SUCCESS;
}


