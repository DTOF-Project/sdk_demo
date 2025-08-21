#include <math.h>
#include "inc/dtof_api.h"
#include "inc/dtof_common.h"
#include "inc/dtof_driver.h"
#include "inc/dtof_float.h"
#include "inc/dtof_endian.h"
#include "inc/dtof_log.h"
#include "inc/dtof_version.h"
#include "inc/dtof_global_config.h"
#include "lib/dtof_lib.h"
#include "src/ramcode/dtof_distance_mode.ram"
#include "src/ramcode/dtof_pre_config.ram"

// #define DTOF_API_DEBUG_FLAG

static dtof_device_info_t dtof_device_info[DTOF_MAX_DEVICE_NUM] =
{
    {.device_id = 0, .is_init = DTOF_FALSE, .first_frame = 1, .frame_id_pre = 0, .distance_offset = 0},
    {.device_id = 1, .is_init = DTOF_FALSE, .first_frame = 1, .frame_id_pre = 0, .distance_offset = 0},
};

/**
 * @brief init device info .
 */
void dtof_init_device_info(dtof_uint8_t device_id)
{
    dtof_device_info[device_id].is_init = DTOF_FALSE;
    dtof_device_info[device_id].first_frame = 1;
    dtof_device_info[device_id].frame_id_pre = 0;
    dtof_device_info[device_id].distance_offset = 0;
}

/**
 * @brief init all device info .
 */
void dtof_init_all_device_info(void)
{
    for(int i = 0; i < DTOF_MAX_DEVICE_NUM; i++)
    {
        dtof_init_device_info(i);
    }
}

void dtof_set_distance_offset(dtof_uint8_t device_id, dtof_int32_t offset)
{
    dtof_device_info[device_id].distance_offset = offset;
}

dtof_int32_t dtof_get_distance_offset(dtof_uint8_t device_id)
{
    return dtof_device_info[device_id].distance_offset;
}

DTOF_RET dtof_set_xtalk_data(dtof_uint8_t device_id, dtof_uint16_t* xtalk_data)
{
#define DTOF_SET_XTALK_DATA_SIZE 36
#define DTOF_SET_XTALK_DATA_END 1996
#define DTOF_XTALK_OFFSET (DTOF_DISTANCE_MODE_CODE_SIZE - 74)

    dtof_uint16_t xtalk_data_set[DTOF_SET_XTALK_DATA_SIZE];
    dtof_uint16_t ram_start = DTOF_XTALK_OFFSET + 0x2000;

    xtalk_data_set[DTOF_SET_XTALK_DATA_SIZE - 1] = DTOF_SET_XTALK_DATA_END;
    xtalk_data_set[DTOF_SET_XTALK_DATA_SIZE - 2] = xtalk_data[XTALK_DATA_SIZE - 1];
    for(int i = 0; i < XTALK_DATA_SIZE - 1; i++)
    {
        xtalk_data_set[2 * i] = xtalk_data[i] & 0xFF;         // 低字节
        xtalk_data_set[2 * i + 1] = (xtalk_data[i] >> 8) & 0xFF;  // 高字节
    }

    DTOF_CHECK_RET(dtof_set_mcu_status(device_id, DTOF_MCU_STATE_SLEEP_DIRECT), "set mcu sleep failed\n");

    DTOF_CHECK_RET(dtof_reg_burst_write(device_id, 0XFE, &ram_start, 1), "write ram start failed\n");
    DTOF_CHECK_RET(dtof_reg_burst_write(device_id, 0xFF, xtalk_data_set, DTOF_SET_XTALK_DATA_SIZE), "write xtalk data failed");

    DTOF_CHECK_RET(dtof_set_mcu_status(device_id, DTOF_MCU_STATE_WAKEUP), "wakeup mcu failed\n");
    return 0;
}

// FIFO读取优化
DTOF_RET dtof_get_fifo(dtof_uint8_t device_id, dtof_start_mode_t dtof_start_mode, dtof_distance_result_t* result_info_p)
{
    DTOF_CHECK_PTR(result_info_p);

    dtof_uint16_t distance_result[DTOF_DISTANCE_RESULT_LEN];

    DTOF_CHECK_RET(dtof_reg_burst_read(device_id, DTOF_SAVE_RESULT_REG_ADDR, distance_result, DTOF_DISTANCE_RESULT_LEN),
                    "读取距离结果失败");

    // 填充结果数据
    result_info_p->frame_id = distance_result[DTOF_FIFO_FRAME_ID];
    result_info_p->first_target = (dtof_int16_t)(distance_result[DTOF_FIFO_DISTANCE0] / DTOF_DISTANCE_RESULT_DIVISOR - DTOF_DISTANCE_RESULT_OFFSET + dtof_device_info[device_id].distance_offset);
    result_info_p->first_intensity = distance_result[DTOF_FIFO_INTENSITY0] >> DTOF_DISTANCE_INTENSITY_SHIFT;
    result_info_p->main_nflash = distance_result[DTOF_FIFO_MAIN_NFLASH];

    // 计算主噪声
    result_info_p->ambient = dtof_div((distance_result[DTOF_FIFO_NOISE_LOW] + ((distance_result[DTOF_FIFO_NOISE_HIGH] & 0x3c) << 14))
                                        , (dtof_real32_t)DTOF_MAIN_NOISE_DIV);

    // 判断是否合法帧
    result_info_p->is_legal_frame = dtof_clac_confidence(result_info_p->first_target, result_info_p->first_intensity, result_info_p->ambient);

    if (dtof_start_mode == NORMAL_DISTANCE_MODE) {
        if (result_info_p->first_target < 0) {
            result_info_p->first_target = 0;
        }
    }

    return DTOF_RET_SUCCESS;
}

DTOF_RET dtof_get_distance_result(dtof_uint8_t device_id, dtof_start_mode_t dtof_start_mode, dtof_distance_result_t* result_info_p, dtof_bool_t *is_new_frame)
{
    DTOF_RET ret;
    dtof_uint16_t frame_id;

    DTOF_CHECK_PTR(result_info_p);

    ret = dtof_get_fifo(device_id, dtof_start_mode, result_info_p);
    if(ret != DTOF_RET_SUCCESS) {
        DTOF_LOG_ERR("dtof get fifo fail\n");
        return ret;
    }
    frame_id = result_info_p->frame_id;
    *is_new_frame = DTOF_FALSE;

    // DTOF_LOG(
    //     "frame_id= %d, frame_id_pre = %d \n",
    //     frame_id, dtof_device_info[device_id].frame_id_pre
    // );

    if (dtof_device_info[device_id].first_frame) {
        // 第一帧不要
        dtof_device_info[device_id].first_frame = 0;
        dtof_device_info[device_id].frame_id_pre = frame_id;
        return DTOF_RET_SUCCESS;
    } else {
        // 检查帧ID是否变化
        if (frame_id == dtof_device_info[device_id].frame_id_pre) {
            // 同一帧
            return DTOF_RET_SUCCESS;
        }

        if (((frame_id - dtof_device_info[device_id].frame_id_pre) != 1) && ((frame_id - dtof_device_info[device_id].frame_id_pre) != 2)) {
            // 异常帧
            dtof_device_info[device_id].first_frame = 1;
            return DTOF_RET_FAILED;
        }
    }
    // 新的一帧
    *is_new_frame = DTOF_TRUE;
    dtof_device_info[device_id].frame_id_pre = frame_id;

    return DTOF_RET_SUCCESS;
}

// MCU状态设置优化
DTOF_RET dtof_set_mcu_status(dtof_uint8_t device_id, dtof_uint32_t status)
{
    dtof_uint16_t temp = 0;
    dtof_uint16_t try_count = 0;

#ifdef DTOF_API_DEBUG_FLAG
    DTOF_LOG("call dtof_set_mcu_status() status = 0x%08x\n", status);
#endif

    if(status != DTOF_MCU_STATE_WAKEUP) {
        DTOF_CHECK_RET(dtof_io_interaction(device_id, status, DTOF_CMD_NONE),
                     "发送IO命令失败");

        // 等待芯片ID就绪
        do {
            DTOF_CHECK_RET(dtof_reg_burst_read(device_id, DTOF_CHIP_ID_REG_ADDR, &temp, 1),
                         "读取寄存器失败");
#ifdef DTOF_API_DEBUG_FLAG
            DTOF_LOG("dtof_set_mcu_status() temp = 0x%04x\n", temp);
#endif

            if (++try_count >= DTOF_MAX_RETRY_COUNT) {
                return DTOF_RET_ERROR;
            }
        } while (temp == DTOF_INVALID_REG_VALUE);
    } else {
        dtof_uint16_t wakeup = 0;
        DTOF_CHECK_RET(dtof_reg_burst_write(device_id, DTOF_IO_CTRL_REG_ADDR, &wakeup, 1),
                     "写入唤醒寄存器失败");

        // 等待寄存器值无效
        do {
            DTOF_CHECK_RET(dtof_reg_burst_read(device_id, DTOF_CHIP_ID_REG_ADDR, &temp, 1),
                         "读取寄存器失败");
#ifdef DTOF_API_DEBUG_FLAG
            DTOF_LOG("dtof_set_mcu_status(WAKEUP) temp = 0x%04x\n", temp);
#endif

            if (++try_count >=DTOF_MAX_RETRY_COUNT) {
                return DTOF_RET_ERROR;
            }
        } while (temp != DTOF_INVALID_REG_VALUE);
    }

    return DTOF_RET_SUCCESS;
}

/**
 * @brief get chip uuid
 * @param uuid uuid buffer
 * @param len uuid buffer length: 16
 * @return DTOF_RET_SUCCESS or DTOF_FAIL
 * @note
 */
DTOF_RET dtof_get_uuid(dtof_uint8_t device_id, dtof_uint8_t *uuid_p, dtof_uint8_t len)
{
    DTOF_RET ret;
    ret = dtof_io_interaction(device_id, DTOF_CMD_GET_UUID, DTOF_CMD_NONE);
    if(ret != DTOF_RET_SUCCESS){
        DTOF_LOG("file: %s, line: %d, send io cmd fail\n", __FILE__, __LINE__);
        return ret;
    }
    ret = dtof_reg_burst_read(device_id, DTOF_SAVE_RESULT_REG_ADDR, (dtof_uint16_t*)uuid_p, len / 2);
    if(ret != DTOF_RET_SUCCESS){
        DTOF_LOG("file: %s, line: %d, reg read fail\n", __FILE__, __LINE__);
        return ret;
    }
    return ret;
}



/**
 * @brief get error status
 * @param status error status
 * @return DTOF_RET_SUCCESS or DTOF_FAIL
 * @note
 */
DTOF_RET dtof_get_error_info(dtof_uint8_t device_id, dtof_uint32_t *status)
{
    DTOF_RET ret;
    dtof_uint16_t reg_data;
    ret = dtof_io_interaction(device_id, DTOF_CMD_SYS_ERROR, DTOF_VAL_SYS_ERROR_GET);
    if(ret != DTOF_RET_SUCCESS){
        DTOF_LOG("file: %s, line: %d, send io cmd fail\n", __FILE__, __LINE__);
        return ret;
    }
    ret = dtof_reg_burst_read(device_id, DTOF_ERROR_INFO_REG_ADDR, &reg_data, 1);
    if(ret != DTOF_RET_SUCCESS){
        DTOF_LOG("file: %s, line: %d, reg read fail\n", __FILE__, __LINE__);
        return ret;
    }
    *status = reg_data;
    return ret;
}

/**
 * @brief clear error status
 * @return DTOF_RET_SUCCESS or DTOF_FAIL
 * @note clear flag
 */
DTOF_RET dtof_clear_error_info(dtof_uint8_t device_id)
{
    DTOF_RET ret;
    ret = dtof_io_interaction(device_id, DTOF_CMD_SYS_ERROR, DTOF_VAL_SYS_ERROR_CLR);
    if(ret != DTOF_RET_SUCCESS){
        DTOF_LOG("file: %s, line: %d, send io cmd fail\n", __FILE__, __LINE__);
        return ret;
    }
    return ret;
}


/**
 * @brief clear eye safety error status
 * @return DTOF_RET_SUCCESS or DTOF_FAIL
 * @note clear eye safety flag and reg
 */
DTOF_RET dtof_clear_eye_safety_error_info(dtof_uint8_t device_id)
{
    DTOF_RET ret;
    ret = dtof_io_interaction(device_id, DTOF_CMD_SYS_ERROR, DTOF_VAL_SYS_ERROR_EYE_SAFETY_CLR);
    if(ret != DTOF_RET_SUCCESS){
        DTOF_LOG("file: %s, line: %d, send io cmd fail\n", __FILE__, __LINE__);
        return ret;
    }
    return ret;
}

/**
 * @brief set io voltage
 * @param value io voltage value, auto-0, 1V2-1, 1V8-2, 3V3-3
 * @return DTOF_RET_SUCCESS or DTOF_FAIL
 * @note
 */
DTOF_RET dtof_set_io_voltage(dtof_uint8_t device_id, dtof_uint8_t value)
{
    DTOF_RET ret;
    ret = dtof_io_interaction(device_id, DTOF_CMD_SET_VCCIO, value);
    if(ret != DTOF_RET_SUCCESS){
        DTOF_LOG("file: %s, line: %d, send io cmd fail\n", __FILE__, __LINE__);
        return ret;
    }
    return ret;
}


/**
 * @brief load ram code
 * @param ram_code ram code data
 * @param len length of ram code data
 * @param mode burn mode 1-2
 * @return DTOF_RET_SUCCESS or DTOF_FAIL
 * @note base on 16k
 */
DTOF_RET dtof_ram_code_burn(dtof_uint8_t device_id, const dtof_uint16_t *ram_code, dtof_uint16_t len, dtof_uint8_t mode, dtof_uint16_t ram_offset)
{
#define DTOF_RAM_CODE_BURN_DELAY 1
    DTOF_RET ret;
    dtof_uint16_t crc_result = 0;
    dtof_uint8_t try_time = 0;
    // dtof_uint16_t temp = 0;

    // support 9000 addr start
    if (mode != (DTOF_SWB_TYPE_FROM_RAM_FIXADDR))
    {
        DTOF_LOG("file: %s, line: %d, ram code burn mode error\n", __FILE__, __LINE__);
        return DTOF_RET_ERROR;
    }

    do{
        // update verison cmd
        ret = dtof_io_interaction(device_id, DTOF_CMD_SWITCH_VERSION, mode);
        if(ret != DTOF_RET_SUCCESS){
            DTOF_LOG("file: %s, line: %d, send io cmd fail\n", __FILE__, __LINE__);
            return ret;
        }

        // wait inner mcu ready
        dtof_sleep_ms(DTOF_RAM_CODE_BURN_DELAY);

        // 烧写程序 iic / spi
        ret = dtof_reg_burst_write_burn(device_id, DTOF_READ_RAM_START_REG_ADDR, ram_code, len);
        if(ret != DTOF_RET_SUCCESS){
            DTOF_LOG("file: %s, line: %d, reg write fail\n", __FILE__, __LINE__);
            return ret;
        }

        // wake up inner mcu
        ret = dtof_set_mcu_status(device_id, DTOF_MCU_STATE_WAKEUP);
        if(ret != DTOF_RET_SUCCESS){
            DTOF_LOG("file: %s, line: %d, wake up mcu fail\n", __FILE__, __LINE__);
            return ret;
        }

        ret = dtof_reg_burst_read(device_id, DTOF_CHECK_CRC_REG_ADDR, &crc_result, 1);
        if(ret != DTOF_RET_SUCCESS){
            DTOF_LOG("file: %s, line: %d, reg read fail\n", __FILE__, __LINE__);
            return ret;
        }

    #ifdef DTOF_L3
        // check crc result (16k)
        if ((crc_result & 0xff) == 0xAA)
        {
            ret = DTOF_RET_SUCCESS;
        }else {
            ret = crc_result & 0xff;
        }
    #endif
        try_time++;

    }while(try_time < 3 && ret != DTOF_RET_SUCCESS);

    return ret;
}

static DTOF_RET dtof_pre_config(dtof_uint8_t device_id)
{
    DTOF_RET ret;

    // dtof_uint16_t ram_code_temp[DTOF_PRE_CONFIG_CODE_SIZE];
    // memcpy(ram_code_temp, dtof_pre_config_data, sizeof(dtof_pre_config_data));

    ret = dtof_ram_code_burn(device_id, (dtof_uint16_t*)dtof_pre_config_data, DTOF_PRE_CONFIG_CODE_SIZE,
                            DTOF_SWB_TYPE_FROM_RAM_FIXADDR, 0);

    if(ret != DTOF_RET_SUCCESS) {
        DTOF_LOG_ERR("Patch1 code burn failed");
        return ret;
    }

    return ret;
}

static DTOF_RET dtof_distance_mode_config(dtof_uint8_t device_id, const dtof_uint16_t* ram_code_p, dtof_uint16_t len)
{
    DTOF_RET ret;

    ret = dtof_ram_code_burn(device_id, ram_code_p, len,
                            DTOF_SWB_TYPE_FROM_RAM_FIXADDR, 0);
    if(ret != DTOF_RET_SUCCESS) {
        DTOF_LOG_ERR("Patch2 code burn failed");
        return ret;
    }

    return ret;
}

DTOF_RET dtof_pre_init(dtof_uint8_t device_id, dtof_uint16_t * chip_id_p)
{
#define DTOF_INIT_DELAY 70
#define DTOF_INIT_FRAME_ID 2
    DTOF_RET ret;
    dtof_uint16_t burn_try_count = 0;
    dtof_uint16_t frame_id;
    dtof_uint16_t reset_value = 0xffff;
    dtof_uint16_t bypass_value = 0x17b9;

    DTOF_CHECK_RET(dtof_reg_burst_write(device_id, DTOF_IO_CTRL_REG_ADDR, &bypass_value, 1), "set mcu sleep failed\n");

    // reset mcu
    do{
        ret = dtof_reg_burst_write(device_id, DTOF_RESET_REG_ADDR, &reset_value, 1);
        if (ret != DTOF_RET_SUCCESS){
            burn_try_count++;
        }
        dtof_sleep_ms(1);

        if (burn_try_count > DTOF_MAX_RETRY_COUNT){
            DTOF_LOG("dtof reset fail\n");
            return ret;
        }
    } while (ret != DTOF_RET_SUCCESS);
    dtof_sleep_ms(DTOF_INIT_DELAY);

    // 等待内部启动完成
    burn_try_count = 0;
    do{
        DTOF_CHECK_WARN(dtof_reg_burst_read(device_id, DTOF_SAVE_RESULT_REG_ADDR, &frame_id, 1), "reg frame id fail\n");
        if(++burn_try_count > DTOF_MAX_RETRY_COUNT){
            DTOF_LOG("dtof init fail\n");
            break;
        }
        dtof_sleep_ms(DTOF_INIT_DELAY);
    } while ((frame_id < DTOF_INIT_FRAME_ID) && (frame_id != 0xffff));

    
    DTOF_CHECK_RET(dtof_reg_burst_read(device_id, DTOF_CHIP_ID_REG_ADDR, chip_id_p, 1), "reg read chip id fail\n");

#ifdef DTOF_API_DEBUG_FLAG
    DTOF_LOG("chip id(before set sleep) = 0x%04x\n", *chip_id_p);
#endif

    DTOF_CHECK_RET(dtof_set_mcu_status(device_id, DTOF_MCU_STATE_SLEEP_DIRECT), "set mcu sleep failed\n");
    DTOF_CHECK_RET(dtof_reg_burst_read(device_id, DTOF_CHIP_ID_REG_ADDR, chip_id_p, 1), "reg read chip id fail\n");

#ifdef DTOF_API_DEBUG_FLAG
    DTOF_LOG("chip id(bypass) = 0x%04x\n", *chip_id_p);
#endif

    DTOF_CHECK_RET(dtof_set_mcu_status(device_id, DTOF_MCU_STATE_WAKEUP), "wakeup mcu failed\n");
    DTOF_LOG("chip id(final) = 0x%04x\n", *chip_id_p);

    ret = dtof_version_upgrade(device_id, 0, 0, 0);
    if(ret != DTOF_RET_SUCCESS){
        return ret;
    }

    return DTOF_RET_SUCCESS;
}

DTOF_RET dtof_set_calibration_data(dtof_uint8_t device_id, dtof_start_mode_t dtof_start_mode)
{
    dtof_uint16_t xtalk_data[XTALK_DATA_SIZE] = {
        6939, 6427, 6682, 7196, 7452, 14634, 14908, 11567, 11567, 
        10027, 9253, 8739, 8225, 7711, 6941, 6426, 1, 11
    };
    dtof_int32_t distance_offset = 0;
    switch(dtof_start_mode)
    {
        case NORMAL_DISTANCE_MODE:
        {
            dtof_get_distance_offset_from_flash(device_id, &distance_offset);
            dtof_set_distance_offset(device_id, distance_offset);
            dtof_get_xtalk_data_from_flash(device_id, xtalk_data);
            dtof_set_xtalk_data(device_id, xtalk_data);
            break;
        }
        case DO_XTALK_CALIBRATION_MODE:
        {
            DTOF_CHECK_RET(dtof_do_xtalk_calibration(device_id, xtalk_data), "do xtalk calibration failed\n");
            dtof_set_xtalk_data_from_flash(device_id, xtalk_data);
            break;
        }
        case DO_OFFSET_CALIBRATION_MODE:
        {
            dtof_get_xtalk_data_from_flash(device_id, xtalk_data);
            dtof_set_xtalk_data(device_id, xtalk_data);
            DTOF_CHECK_RET(dtof_do_distance_calibration(device_id, &distance_offset), "do distance calibration failed\n");
            break;
        }
        default:
        {
            DTOF_LOG_ERR("unknown start mode: %d\n", dtof_start_mode);
            return DTOF_RET_FAILED;
        }
    }
    return DTOF_RET_SUCCESS;
}

DTOF_RET dtof_init_and_wait_for_ready(dtof_uint8_t device_id, dtof_uint16_t * chip_id_p, dtof_start_mode_t dtof_start_mode)
{

    if (dtof_device_info[device_id].is_init == DTOF_FALSE){
        DTOF_CHECK_RET(dtof_pre_init(device_id, chip_id_p), "dtof pre init failed\n");
        dtof_device_info[device_id].is_init = DTOF_TRUE;
    }

    DTOF_CHECK_RET(dtof_set_calibration_data(device_id, dtof_start_mode), "set calibration data failed\n");

    return DTOF_RET_SUCCESS;
}

/**
 * @brief This function is used to initialize the sensor.
 * @return DTOF_RET_SUCCESS if the initialization is successful, DTOF_RET_FAILED otherwise.
 */
DTOF_RET dtof_version_upgrade(dtof_uint8_t device_id, dtof_uint8_t *uuid_p, dtof_uint8_t *ft_data_p, dtof_uint16_t len)
{
#define DTOF_RAM_JUDGE_RES_DELAY   1
#define DTOF_FIRST_BURN_END_FLAG  0xA1ED
#define DTOF_SECOND_BURN_END_FLAG  0xA2ED
#define DTOF_FT_DATA_OFFSET        (DTOF_DISTANCE_MODE_CODE_SIZE - 104)
    DTOF_RET ret;
    dtof_uint16_t burn_end_flag = 0;
    dtof_uint16_t burn_try_count = 0;
    dtof_uint16_t burn_judge_count = 0;

    do{
        ret = dtof_pre_config(device_id);
        if(!ret){
            // 判断升级结束flag
            burn_judge_count = 0;
            do{
                DTOF_CHECK_RET(dtof_reg_burst_read(device_id, DTOF_CHECK_UPGRADE_REG_ADDR, &burn_end_flag, 1), "reg read fail\n");
                if(burn_end_flag == DTOF_FIRST_BURN_END_FLAG){
                    break;
                }
                dtof_sleep_ms(DTOF_RAM_JUDGE_RES_DELAY);
            }while(++burn_judge_count <= DTOF_MAX_RETRY_COUNT);
        }

        if(++burn_try_count >= DTOF_MAX_RETRY_COUNT){
            DTOF_LOG_ERR("patch1 burn fail\n");
            return DTOF_RET_FAILED;
        }
    }while((burn_try_count != DTOF_MAX_RETRY_COUNT) && (burn_end_flag != DTOF_FIRST_BURN_END_FLAG));

    ret = dtof_set_mcu_status(device_id, DTOF_MCU_STATE_WAKEUP);
    if(ret != DTOF_RET_SUCCESS){
        DTOF_LOG_ERR("set mcu status fail\n");
        return ret;
    }

    // generate version
    // dtof_uint16_t ram_code_temp[DTOF_DISTANCE_MODE_CODE_SIZE];

    // memcpy(ram_code_temp, dtof_distance_mode_data, sizeof(dtof_distance_mode_data));
    // // patch2数据填充
    // // 这里根据 len 是不是为0 来判断要不要 做数据的填充
    // // 如果 len 为0, 则不填充数据
    // // 如果 len 不为0, 则填充数据
    // // 填充数据到 ram_code_temp 中
    // // 默认的时候 ram_code_temp 中的数据是空的  是 全0
    // // ram 代码需要判断是 0 的时候 不做任何事情
    // if (len > (0)){
    //     DTOF_CHECK_PTR(ft_data_p);
    //     DTOF_CHECK_PTR(uuid_p);

    //     // check uuid
    //     if (memcmp(uuid_p, ft_data_p, DTOF_UUID_LENGTH) != 0)
    //     {
    //         DTOF_LOG_ERR("uuid check failed\n");
    //         return DTOF_RET_FAILED;
    //     }

    //     memcpy(ram_code_temp + DTOF_FT_DATA_OFFSET, ft_data_p + DTOF_UUID_LENGTH, len * 2);
    // }

    burn_try_count = 0;
    do{
        ret = dtof_distance_mode_config(device_id, dtof_distance_mode_data, DTOF_DISTANCE_MODE_CODE_SIZE);
        if(!ret){
            // 判断升级结束flag
            burn_judge_count = 0;
            do{
                DTOF_CHECK_RET(dtof_reg_burst_read(device_id, DTOF_CHECK_UPGRADE_REG_ADDR, &burn_end_flag, 1), "reg read fail\n");
                if(burn_end_flag == DTOF_SECOND_BURN_END_FLAG){
                    break;
                }
                dtof_sleep_ms(DTOF_RAM_JUDGE_RES_DELAY);
            }while(++burn_judge_count <= DTOF_MAX_RETRY_COUNT);
        }

        if(++burn_try_count >= DTOF_MAX_RETRY_COUNT){
            DTOF_LOG_ERR("patch2 burn fail\n");
            return DTOF_RET_FAILED;
        }
    }while((burn_try_count != DTOF_MAX_RETRY_COUNT) && (burn_end_flag != DTOF_SECOND_BURN_END_FLAG));

    return ret;
}

/**
 * @brief This function is used to start the distance measurement.
 *
 * It will set the mcu status to sleep first, then write the start flag to the frame control register,
 * and finally set the mcu status to wakeup.
 *
 * @return DTOF_RET_SUCCESS if the start is successful, DTOF_RET_FAILED otherwise.
 */
DTOF_RET dtof_start_distance_measure(dtof_uint8_t device_id)
{
    dtof_uint16_t start_flag = DTOF_START_DISATNCE_MODE;

    DTOF_CHECK_RET(dtof_set_mcu_status(device_id, DTOF_MCU_STATE_SLEEP_DIRECT), "set mcu sleep failed\n");
    DTOF_CHECK_RET(dtof_reg_burst_write(device_id, DTOF_FRAME_CONTROL_REG, &start_flag, 1), "reg write fail\n");
    DTOF_CHECK_RET(dtof_set_mcu_status(device_id, DTOF_MCU_STATE_WAKEUP), "wakeup mcu failed\n");

    return DTOF_RET_SUCCESS;
}

/**
 * @brief This function is used to stop the distance measurement.
 *
 * It will set the mcu status to sleep first, then write the stop flag to the frame control register,
 * and finally set the mcu status to wakeup.
 *
 * @return DTOF_RET_SUCCESS if the stop is successful, DTOF_RET_FAILED otherwise.
 */

DTOF_RET dtof_stop_distance_measure(dtof_uint8_t device_id)
{
    dtof_uint16_t stop_flag = DTOF_STOP_DISATNCE_MODE;

    DTOF_CHECK_RET(dtof_set_mcu_status(device_id, DTOF_MCU_STATE_SLEEP_DIRECT), "set mcu sleep failed\n");
    DTOF_CHECK_RET(dtof_reg_burst_write(device_id, DTOF_FRAME_CONTROL_REG, &stop_flag, 1), "reg write fail\n");
    DTOF_CHECK_RET(dtof_set_mcu_status(device_id, DTOF_MCU_STATE_WAKEUP), "wakeup mcu failed\n");

    return DTOF_RET_SUCCESS;
}

/**
 * @brief This function is used to quit the distance measurement.
 *
 * It will set the mcu status to sleep first, then write the quit flag to the frame control register,
 * and finally set the mcu status to wakeup.
 *
 * @return DTOF_RET_SUCCESS if the quit is successful, DTOF_RET_FAILED otherwise.
 */

DTOF_RET dtof_quit_distance_measure(dtof_uint8_t device_id)
{
    dtof_uint16_t quit_flag = DTOF_QUIT_DISATNCE_MODE;

    DTOF_CHECK_RET(dtof_set_mcu_status(device_id, DTOF_MCU_STATE_SLEEP_DIRECT), "set mcu sleep failed\n");
    DTOF_CHECK_RET(dtof_reg_burst_write(device_id, DTOF_FRAME_CONTROL_REG, &quit_flag, 1), "reg write fail\n");
    DTOF_CHECK_RET(dtof_set_mcu_status(device_id, DTOF_MCU_STATE_WAKEUP), "wakeup mcu failed\n");

    return DTOF_RET_SUCCESS;
}

DTOF_RET dtof_do_distance_calibration(dtof_uint8_t device_id, dtof_int32_t *distance_offset)
{
#define DTOF_DO_CALIBRATION_DISTANCE 20 // 20mm
#define DTOF_DO_CALIBRATION_IGNORE_FRAME 50
#define DTOF_DO_CALIBRATION_SAMPLE_FRAME 100
#define DTOF_DO_CALIBRATION_MAX_FRAME 200
    DTOF_RET ret;
    dtof_uint32_t legal_frame_cnt = 0;
    dtof_int32_t distance_sum = 0;
    dtof_int16_t distance_avg;
    dtof_bool_t is_new_flag;
    dtof_distance_result_t distance_result;
    // 恢复默认B value
    dtof_set_distance_offset(device_id, 0);

    DTOF_CHECK_RET(dtof_start_distance_measure(device_id), "start distance measure failed\n");

    while (legal_frame_cnt < (DTOF_DO_CALIBRATION_IGNORE_FRAME + DTOF_DO_CALIBRATION_SAMPLE_FRAME))
    {
        DTOF_CHECK_WARN(dtof_get_distance_result(device_id, DO_OFFSET_CALIBRATION_MODE, &distance_result, &is_new_flag), "get distance result failed\n");

        // 检查芯片ID是否有效
        if (is_new_flag != DTOF_TRUE || distance_result.is_legal_frame != 1)
            continue;
        // 有效帧增加

        if (legal_frame_cnt >= DTOF_DO_CALIBRATION_IGNORE_FRAME)
        {
            distance_sum += distance_result.first_target;
        }
        legal_frame_cnt++;
    }

    DTOF_CHECK_RET(dtof_stop_distance_measure(device_id), "stop distance measure failed\n");

    distance_avg = distance_sum / DTOF_DO_CALIBRATION_SAMPLE_FRAME;

    *distance_offset = DTOF_DO_CALIBRATION_DISTANCE - distance_avg;

    dtof_set_distance_offset(device_id, *distance_offset);

    dtof_set_distance_offset_to_flash(device_id, *distance_offset);

    return DTOF_RET_SUCCESS;
}

const char* dtof_get_sdk_version(void)
{
    return DTOF_VERSION_STRING;
}

dtof_uint16_t dtof_get_chip_version(dtof_uint8_t device_id)
{
    dtof_uint16_t sdk_version = dtof_distance_mode_data[DTOF_DISTANCE_MODE_CODE_SIZE-2];
    return sdk_version;
}
