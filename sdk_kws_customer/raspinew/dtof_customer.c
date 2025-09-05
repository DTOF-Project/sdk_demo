#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <sys/stat.h>

#include "i2c_init.h"
#include "dtof_customer.h"
#include "sdk/inc/dtof_base_type.h"
#include "sdk/inc/dtof_endian.h"
#include "sdk/inc/dtof_log.h"

extern device_driver_ops_t device_iic_driver_ops;

static void dtof_convert_endian(uint16_t *data, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++) {
        data[i] = DTOF_SWAP16(data[i]);
    }
}

DTOF_RET dtof_reg_burst_write(uint8_t device_id, uint8_t reg_addr, uint16_t *reg_data_p, uint16_t len) {

#ifdef DEBUG_LOG_FLAG
    DTOF_LOG("write 0x%02x reg_data_p[0] = 0x%04x\n", reg_addr, reg_data_p[0]);
#endif

    // 主机uint16 -> 从机大端序uint8
    // TODO： 因为目前树莓派上只有 iic 的接口，所以 先不判断是什么连接方式，直接先转大小端
    dtof_convert_endian(reg_data_p, len);

#ifdef DEBUG_LOG_FLAG
    // 大端序
    DTOF_LOG("write 0x%02x input_data_p[0], [1] = 0x%02x, 0x%02x\n", reg_addr, input_data_p[0], input_data_p[1]);
    // 小端序
    // DTOF_LOG("write 0x%02x input_data_p[1], [0] = 0x%02x, 0x%02x\n", reg_addr, input_data_p[1], input_data_p[0]);
#endif

    // 调用底层写函数
    int ret = device_iic_driver_ops.write_block(device_id, reg_addr, (uint8_t *)reg_data_p, len);

    // 写完转回
    dtof_convert_endian(reg_data_p, len);

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

// #ifdef DEBUG_LOG_FLAG
//     DTOF_LOG("read 0x%02x reg_data_p[0] (src) = 0x%04x\n", reg_addr, reg_data_p[0]);
// #endif

    // 从机大端序uint8 -> 主机uint16
    if (ret == DTOF_RET_SUCCESS){
        dtof_convert_endian(reg_data_p, len);
    }

#ifdef DEBUG_LOG_FLAG
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



// 确保数据目录存在
static int ensure_data_directory_exists(void)
{
    struct stat st = {0};
    if (stat("./data", &st) == -1) {
        // 目录不存在，尝试创建
        if (mkdir("./data", 0755) == -1) {
            perror("创建数据目录失败");
            return DTOF_RET_FAILED;
        }
    }
    return DTOF_RET_SUCCESS;
}

#define FILE_BUFFER_SIZE 19

static int file_read(dtof_uint8_t uuid, dtof_int32_t *read_buf)
{
    char file_path[256];
    snprintf(file_path, sizeof(file_path), "./data/data_%02x.bin", uuid);

    FILE *filePointer = fopen(file_path, "rb");
    if (filePointer == NULL) {
        // 文件不存在不是错误，可能是第一次使用
        return DTOF_RET_FAILED;
    }
    
    // 读取文件内容
    size_t elements_read = fread(read_buf, sizeof(dtof_int32_t), FILE_BUFFER_SIZE, filePointer);
    fclose(filePointer);
    
    // 检查是否读取了足够的数据
    if (elements_read != FILE_BUFFER_SIZE) {
        DTOF_LOG("警告: 文件 %s 数据不完整，只读取了 %zu/%d 个元素\n", 
            file_path, elements_read, FILE_BUFFER_SIZE);
        return DTOF_RET_FAILED;
    }
    
    return DTOF_RET_SUCCESS;
}

static int file_write(dtof_uint8_t uuid, const dtof_int32_t *write_buf)
{
    // 确保数据目录存在
    if (ensure_data_directory_exists() != DTOF_RET_SUCCESS) {
        return DTOF_RET_FAILED;
    }
    
    char file_path[256];
    snprintf(file_path, sizeof(file_path), "./data/data_%02x.bin", uuid);
    
    // 如果write_buf全为0，则删除文件
    int all_zero = 1;
    for (int i = 0; i < FILE_BUFFER_SIZE; i++) {
        if (write_buf[i] != 0) {
            all_zero = 0;
            break;
        }
    }
    
    if (all_zero) {
        if (remove(file_path) == 0) {
            return DTOF_RET_SUCCESS;
        } else if (errno != ENOENT) { // 文件不存在不是错误
            perror("删除文件失败");
            return DTOF_RET_FAILED;
        }
        return DTOF_RET_SUCCESS;
    }
    
    // 打开文件进行写入
    FILE *filePointer = fopen(file_path, "wb");
    if (filePointer == NULL) {
        perror("打开文件失败");
        return DTOF_RET_FAILED;
    }
    
    // 写入数据
    size_t elements_written = fwrite(write_buf, sizeof(dtof_int32_t), FILE_BUFFER_SIZE, filePointer);
    fclose(filePointer);
    
    // 检查是否成功写入所有数据
    if (elements_written != FILE_BUFFER_SIZE) {
        DTOF_LOG("错误: 只写入了 %zu/%d 个元素到文件 %s\n", 
                elements_written, FILE_BUFFER_SIZE, file_path);
        return DTOF_RET_FAILED;
    }
    
    return DTOF_RET_SUCCESS;
}

DTOF_RET dtof_get_distance_offset_from_flash(dtof_uint8_t device_id, dtof_int32_t *distance_offset)
{
    DTOF_LOG("on raspi, get offset from file");
    if (distance_offset == NULL) {
        return DTOF_RET_FAILED;
    }
    
    dtof_int32_t buffer[FILE_BUFFER_SIZE] = {0};
    int ret = file_read(device_id, buffer);
    if (ret == DTOF_RET_SUCCESS) {
        *distance_offset = buffer[0];
    }
    
    return ret;
}

DTOF_RET dtof_set_distance_offset_to_flash(dtof_uint8_t device_id, dtof_int32_t distance_offset)
{
    DTOF_LOG("on raspi, save offset to file");
    dtof_int32_t buffer[FILE_BUFFER_SIZE] = {0};
    
    // 先读取现有数据（如果存在）
    file_read(device_id, buffer);
    
    // 更新距离偏移值
    buffer[0] = distance_offset;
    
    return file_write(device_id, buffer);
}

DTOF_RET dtof_get_xtalk_data_from_flash(dtof_uint8_t device_id, dtof_uint16_t *xtalk_data)
{
    DTOF_LOG("on raspi, get xtalk from file");
    if (xtalk_data == NULL) {
        return DTOF_RET_FAILED;
    }
    
    dtof_int32_t buffer[FILE_BUFFER_SIZE] = {0};
    int ret = file_read(device_id, buffer);
    
    if (ret == DTOF_RET_SUCCESS) {
        for (int i = 0; i < 18; i++) {
            // 确保值在uint16_t范围内
            if (buffer[i + 1] < 0) {
                xtalk_data[i] = 0;
            } else if (buffer[i + 1] > 0xFFFF) {
                xtalk_data[i] = 0xFFFF;
            } else {
                xtalk_data[i] = (dtof_uint16_t)buffer[i + 1];
            }
        }
    } else {
        // 提供默认值（如果文件不存在）
        dtof_uint16_t xtalk_data_default[] = {
            11, 258, 257, 514, 514, 514, 
            1284, 1285, 772, 1028, 771, 771, 
            514, 514, 514, 514, 258, 1
        };
        memcpy(xtalk_data, xtalk_data_default, sizeof(xtalk_data_default));
    }
    
    return DTOF_RET_SUCCESS;
}

DTOF_RET dtof_set_xtalk_data_from_flash(dtof_uint8_t device_id, dtof_uint16_t *xtalk_data)
{
    DTOF_LOG("on raspi, save xtalk to file");
    if (xtalk_data == NULL) {
        return DTOF_RET_FAILED;
    }
    
    dtof_int32_t buffer[FILE_BUFFER_SIZE] = {0};
    
    // 先读取现有数据（如果存在）
    file_read(device_id, buffer);
    
    // 更新xtalk数据
    for (int i = 0; i < 18; i++) {
        buffer[i + 1] = xtalk_data[i];
    }
    
    return file_write(device_id, buffer);
}
