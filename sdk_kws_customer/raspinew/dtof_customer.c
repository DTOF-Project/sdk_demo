#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <sys/stat.h>
#include "sdk/inc/dtof_driver.h"
#include "i2c_init.h"
#include "dtof_customer.h"
#include "sdk/inc/dtof_base_type.h"
#include "sdk/inc/dtof_endian.h"
#include "sdk/inc/dtof_log.h"
#include "sdk/inc/dtof_global_config.h"
#include "dtof_reg.h"
#include "sdk/inc/dtof_api.h"
extern device_driver_ops_t device_iic_driver_ops;

static void dtof_convert_endian(uint16_t *data, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++) {
        data[i] = DTOF_SWAP16(data[i]);
    }
}





extern dtof_chip_config_t dtof_chip_config;


#define FT_DATA_NUM (sizeof(dtof_ft_cali_param_t)/sizeof(dtof_uint16_t))
#define FILE_BUFFER_SIZE FT_DATA_NUM




DTOF_RET dtof_reg_burst_write( uint8_t reg_addr, uint16_t *reg_data_p, uint16_t len) {

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
    int ret = device_iic_driver_ops.write_block(1, reg_addr, (uint8_t *)reg_data_p, len);

    // 写完转回
    dtof_convert_endian(reg_data_p, len);

    if (ret == DTOF_RET_ERROR) {
        perror("dtof_reg_burst_write() WRITE ERROR");
    }
    return ret;
}

DTOF_RET dtof_reg_burst_write_burn( uint8_t reg_addr, const uint16_t *reg_data_p, uint16_t len) {
    // write 和 write_burn 的区别: 不需要再次大小端转换，烧写处理好（包括大小端转换）的程序

    // 调用底层写函数
    int ret = device_iic_driver_ops.write_block(1, reg_addr, (uint8_t *)reg_data_p, len);

    if (ret == DTOF_RET_ERROR) {
        perror("dtof_reg_burst_write_burn() WRITE ERROR");
    }
    return ret;
}

DTOF_RET dtof_reg_burst_read( uint8_t reg_addr, uint16_t *reg_data_p, uint16_t len) {
    int ret = device_iic_driver_ops.read_block(1, reg_addr, (uint8_t *)reg_data_p, len);

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

// #define FILE_BUFFER_SIZE 19

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

static int file_read_2(dtof_uint8_t uuid[16], dtof_run_mode_e runmode, dtof_int32_t *read_buf, dtof_bool_t *is_legal_data)
{
   
    char file_path[256];
    char uuid_str[33] = {0}; // 16字节 * 2个字符/字节 + 终止符 '\0'
    for (int i = 0; i < 16; i++) 
    {
        snprintf(uuid_str + i * 2, 3, "%02x", uuid[i]); // 每个字节格式化为2位十六进制
    }

    

    snprintf(file_path, sizeof(file_path), "./data/mode%d_%s.bin", runmode, uuid_str);

   

    FILE *filePointer = fopen(file_path, "rb");
    if (filePointer == NULL) {
        *is_legal_data = DTOF_FALSE;
        return DTOF_RET_SUCCESS;
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
    *is_legal_data = DTOF_TRUE;
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

static int file_write_2(dtof_uint8_t uuid[16], dtof_run_mode_e runmode, const dtof_int32_t *write_buf)
{
    // 确保数据目录存在
    if (ensure_data_directory_exists() != DTOF_RET_SUCCESS) {
        
        return DTOF_RET_FAILED;
    }
    
    char file_path[256];
   char uuid_str[33] = {0}; // 16字节 * 2个字符/字节 + 终止符 '\0'
    for (int i = 0; i < 16; i++) 
    {
    snprintf(uuid_str + i * 2, 3, "%02x", uuid[i]); // 每个字节格式化为2位十六进制
     }
    snprintf(file_path, sizeof(file_path), "./data/mode%d_%s.bin", runmode, uuid_str);

    
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

// deperate
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

DTOF_RET dtof_set_xtalk_data_from_flash(dtof_uint8_t device_id, dtof_uint16_t *xtalk_data, dtof_int16_t pos_cal_result, dtof_uint16_t maxratio_cal_result)
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


DTOF_RET dtof_read_otp(dtof_uint8_t offset, dtof_uint8_t *buf, dtof_uint16_t len)
{
    dtof_uint16_t reg3;
    dtof_addressREG3_t *reg3_p = (dtof_addressREG3_t *)&reg3;
    dtof_uint16_t cfgdone_backup;
    dtof_uint16_t ram_start;
    dtof_uint16_t ram_start_backup;
    dtof_uint16_t temp;
    ram_start = 0x800 + offset;

    // disable cfgdone
    DTOF_CHECK_RET(dtof_reg_burst_read(DTOF_REG3, &reg3, 1), "read reg 3 fail\n");
    cfgdone_backup = reg3_p->cfgDoneR;
    reg3_p->cfgDoneR = 0;
    DTOF_CHECK_RET(dtof_reg_burst_write(DTOF_REG3, &reg3, 1), "write reg 3 fail\n");

    // set otp start addr
    DTOF_CHECK_RET(dtof_reg_burst_read(DTOF_REG254, &ram_start_backup, 1), "read reg 254 fail\n");
    DTOF_CHECK_RET(dtof_reg_burst_write(DTOF_REG254, &ram_start, 1), "write reg 254 fail\n");

    // read otp
    for (dtof_uint16_t i = 0; i < len; i++)
    {
        DTOF_CHECK_RET(dtof_reg_burst_read(DTOF_REG255, &temp, 1), "read reg 255 fail\n");
        *(buf + i) =  temp & 0xff;
    }

    // restore reg
    DTOF_CHECK_RET(dtof_reg_burst_write(DTOF_REG254, &ram_start_backup, 1), "write reg 254 fail\n");
    reg3_p->cfgDoneR = cfgdone_backup;
    DTOF_CHECK_RET(dtof_reg_burst_write(DTOF_REG3, &reg3, 1), "write reg 3 fail\n");

	return DTOF_RET_SUCCESS;
}




DTOF_RET dtof_get_ft_data_from_flash_multi_mode(dtof_uint16_t *ft_data, dtof_uint16_t len, dtof_run_mode_e run_mode, dtof_bool_t *is_legal_data){
    // 参数合法性检查
    if (ft_data == NULL || is_legal_data == NULL) {
        return DTOF_RET_INVALID_PARAM;  // 无效参数
    }
    // 判断 len > FT_DATA_NUM， 边界为FT 的最大值
    if (len > FT_DATA_NUM) {
        return DTOF_RET_LIMIT;  // 缓冲区不足
    }

    // 初始化 合法flag 的锁，当判断数据有效且写入成功时，表示成功
    *is_legal_data = DTOF_FALSE;
    dtof_chip_config_t *chip_cfg = dtof_get_chip_config();
    if (chip_cfg == NULL) {
        return DTOF_RET_ERROR;
    }

    // 定义file_read的读取缓冲区（dtof_int32_t类型，与file_read参数匹配）
    dtof_int32_t read_buf[FT_DATA_NUM];
    // dtof_int32_t write_buf[FILE_BUFFER_SIZE2];
    // int write_ret =file_write_2(chip_cfg->chip_uuid,write_buf);

    // 调用file_read读取数据, 以此文件是否为空，即是否可以读取作为判断依据即可
    int read_ret = file_read_2(chip_cfg->chip_uuid, run_mode, read_buf,is_legal_data);
    if (read_ret != DTOF_RET_SUCCESS) {
        
        return DTOF_RET_ERROR;  
    }
    // *is_legal_data = DTOF_TRUE;


    // 若数据合法，转换为dtof_uint16_t存入输出缓冲区（取低16位，或直接强转，根据实际存储格式）
    if (*is_legal_data == DTOF_TRUE) {
        for (dtof_uint16_t i = 0; i < FT_DATA_NUM; i++) {
            // 假设文件中int32_t的低16位为有效数据（根据实际存储逻辑调整转换方式）
            ft_data[i] = (dtof_uint16_t)(read_buf[i] & 0xFFFF);
        }
    }

    return DTOF_RET_SUCCESS;
}

DTOF_RET dtof_set_ft_data_to_flash_multi_mode(dtof_uint16_t *ft_data, dtof_uint16_t len, dtof_run_mode_e run_mode){
    // 参数合法性检查
    if (ft_data == NULL) {
        return DTOF_RET_INVALID_PARAM;  // 无效参数
    }
    // 判断 len > FT_DATA_NUM， 边界为FT 的最大值
    if (len > FT_DATA_NUM) {
        return DTOF_RET_LIMIT;  // 缓冲区不足
    }

    // 抓取chip id，
    dtof_chip_config_t *chip_cfg = dtof_get_chip_config();
    if (chip_cfg == NULL) {
        return DTOF_RET_ERROR;
    }
    dtof_int32_t write_buf[FT_DATA_NUM];

    for(dtof_uint16_t i = 0; i < FT_DATA_NUM; i++)
    {
        write_buf[i] = (dtof_int32_t)(*(ft_data + i));
    }

    
    int write_ret =file_write_2(chip_cfg->chip_uuid, run_mode, write_buf);
    if (write_ret != DTOF_RET_SUCCESS){
        return DTOF_RET_ERROR;
    }
    return DTOF_RET_SUCCESS;
}