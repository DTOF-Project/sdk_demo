#include <math.h>
#include "inc/dtof_common.h"
#include "inc/dtof_driver.h"
#include "inc/dtof_float.h"
#include "inc/dtof_endian.h"
#include "inc/dtof_log.h"
#include "inc/dtof_global_config.h"
#include "inc/dtof_calibration_ft.h"
#include "inc/dtof_api.h"
#include "src/ramcode/dtof_version.h"

static dtof_device_info_t dtof_device_info =
{
    .chip_is_init = DTOF_FALSE,
#ifdef DTOF_POLLING_MODE
    .first_frame = 1,
    .frame_id_pre = 0,
#endif
};

void dtof_init_device_info(void)
{
    dtof_device_info.chip_is_init = DTOF_FALSE;
#ifdef DTOF_POLLING_MODE
    dtof_device_info.first_frame = 1;
    dtof_device_info.frame_id_pre = 0;
#endif
}

/**
 * @brief Get distance measurement results from fifo.
 * @param result_info_p Pointer to the structure to store the distance result.
 * @return DTOF_RET_SUCCESS on success, or an error code on failure.
 */
DTOF_RET dtof_get_distance_result(dtof_distance_result_t *result_info_p)
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

    return DTOF_RET_SUCCESS;
}

#ifdef DTOF_POLLING_MODE
/**
 * @brief Get distance measurement results in polling mode.
 * @param result_info_p Pointer to the structure to store the distance result.
 * @param is_new_frame Pointer to a boolean indicating if a new frame is available.
 * @return DTOF_RET_SUCCESS on success, or an error code on failure.
 */
DTOF_RET dtof_get_distance_result_polling(dtof_distance_result_t *result_info_p, dtof_bool_t *is_new_frame)
{
    DTOF_RET ret;
    dtof_uint16_t frame_id;

    DTOF_CHECK_PTR(result_info_p);

    ret = dtof_get_distance_result(result_info_p);
    if (ret != DTOF_RET_SUCCESS)
    {
        DTOF_LOG_ERR("dtof get fifo fail\n");
        return ret;
    }
    frame_id = result_info_p->frame_id;
    *is_new_frame = DTOF_FALSE;

    if (dtof_device_info.first_frame)
    {
        // 第一帧不要
        dtof_device_info.first_frame = 0;
        dtof_device_info.frame_id_pre = frame_id;
        return DTOF_RET_SUCCESS;
    }
    else
    {
        // 检查帧ID是否变化
        if (frame_id == dtof_device_info.frame_id_pre)
        {
            // 同一帧
            return DTOF_RET_SUCCESS;
        }

        if (((frame_id - dtof_device_info.frame_id_pre) != 1) && ((frame_id - dtof_device_info.frame_id_pre) != 2))
        {
            // 异常帧
            dtof_device_info.first_frame = 1;
            return DTOF_RET_FAILED;
        }
    }
    // 新的一帧
    *is_new_frame = DTOF_TRUE;
    dtof_device_info.frame_id_pre = frame_id;

    return DTOF_RET_SUCCESS;
}
#endif

/**
 * @brief read register value
 * @param reg_addr register address
 * @param reg_data register value
 * @return DTOF_RET_SUCCESS or DTOF_FAIL
 * @note
 */
DTOF_RET dtof_read_reg_running(dtof_uint16_t reg_addr, dtof_uint16_t *reg_data)
{
#define DTOF_RUNNING_READ_REG_ADDR 0x6E
    DTOF_RET ret;
    ret = dtof_io_interaction(DTOF_CMD_READ_REG, reg_addr);
    if (ret != DTOF_RET_SUCCESS)
    {
        DTOF_LOG("file: %s, line: %d, send io cmd fail\n", __FILE__, __LINE__);
        return ret;
    }

    ret = dtof_reg_burst_read(DTOF_RUNNING_READ_REG_ADDR, reg_data, 1);
    if (ret != DTOF_RET_SUCCESS)
    {
        DTOF_LOG("file: %s, line: %d, reg read fail\n", __FILE__, __LINE__);
        return ret;
    }

    return ret;
}

/**
 * @brief write register value
 * @param reg_addr register address
 * @param reg_data register value
 * @return DTOF_RET_SUCCESS or DTOF_FAIL
 * @note
 */
DTOF_RET dtof_write_reg_running(dtof_uint16_t reg_addr, dtof_uint16_t reg_data)
{
    DTOF_RET ret;

    ret = dtof_io_interaction(DTOF_CMD_WRITE_REG_ADDR, reg_addr);
    if (ret != DTOF_RET_SUCCESS)
    {
        DTOF_LOG("file: %s, line: %d, send io cmd fail\n", __FILE__, __LINE__);
        return ret;
    }

    ret = dtof_io_interaction(DTOF_CMD_WRITE_REG_LOW, reg_data & 0xFF);
    if (ret != DTOF_RET_SUCCESS)
    {
        DTOF_LOG("file: %s, line: %d, send io cmd fail\n", __FILE__, __LINE__);
        return ret;
    }

    ret = dtof_io_interaction(DTOF_CMD_WRITE_REG_HIGH, (reg_data >> 8) & 0xFF);
    if (ret != DTOF_RET_SUCCESS)
    {
        DTOF_LOG("file: %s, line: %d, send io cmd fail\n", __FILE__, __LINE__);
        return ret;
    }

    return ret;
}

/**
 * @brief Sets the MCU status to a specified state.
 * @param status The desired MCU status to set, sleep or wakeup.
 * @return DTOF_RET_SUCCESS or DTOF_FAIL
 */
DTOF_RET dtof_set_mcu_status(dtof_uint32_t status)
{
    dtof_uint16_t temp = 0;
    dtof_uint16_t try_count = 0;

    if (status != DTOF_MCU_STATE_WAKEUP)
    {
        DTOF_CHECK_RET(dtof_io_interaction(status, DTOF_CMD_NONE),
                       "发送IO命令失败");

        // 等待芯片ID就绪
        do
        {
            DTOF_CHECK_RET(dtof_reg_burst_read(DTOF_CHIP_ID_REG_ADDR, &temp, 1),
                           "读取寄存器失败");

            if (++try_count >= DTOF_MAX_RETRY_COUNT)
            {
                return DTOF_RET_ERROR;
            }
        } while (temp == DTOF_INVALID_REG_VALUE);
    }
    else
    {
        dtof_uint16_t wakeup = 0;
        DTOF_CHECK_RET(dtof_reg_burst_write(DTOF_IO_CTRL_REG_ADDR, &wakeup, 1),
                       "写入唤醒寄存器失败");

        // 等待寄存器值无效
        do
        {
            DTOF_CHECK_RET(dtof_reg_burst_read(DTOF_CHIP_ID_REG_ADDR, &temp, 1),
                           "读取寄存器失败");

            if (++try_count >= DTOF_MAX_RETRY_COUNT)
            {
                return DTOF_RET_ERROR;
            }
        } while (temp != DTOF_INVALID_REG_VALUE);
    }

    return DTOF_RET_SUCCESS;
}

DTOF_RET dtof_set_mcu_status_ram(dtof_uint32_t status)
{
    dtof_uint16_t temp;
    dtof_uint16_t try_count = 0;
    if (status != DTOF_MCU_STATE_WAKEUP)
    {
        DTOF_CHECK_RET(dtof_write_reg_running(DTOF_FRAME_CONTROL_REG, DTOF_SET_MCU_SLEEP_MODE), "dtof set mcu sleep failed\n");
        do
        {
            DTOF_CHECK_RET(dtof_reg_burst_read(DTOF_ERROR_INFO_REG_ADDR, &temp, 1), "read reg failed\n");
            if (++try_count >= DTOF_MAX_RETRY_COUNT)
            {
                return DTOF_RET_ERROR;
            }
        } while (temp != DTOF_SLEEP_FLAG);

        temp = DTOF_SET_MCU_SLEEP_VALUE;
        DTOF_CHECK_RET(dtof_reg_burst_write(DTOF_IO_CTRL_REG_ADDR, &temp, 1), "write reg failed\n");
    }
    else
    {
#define DTOF_WAKEUP_VALUE_RAM 0x30
        dtof_uint16_t wakeup = 0;
        DTOF_CHECK_RET(dtof_set_mcu_status(DTOF_MCU_STATE_WAKEUP), "wakeup mcu failed\n");
        DTOF_CHECK_RET(dtof_io_interaction(DTOF_WAKEUP_VALUE_RAM, wakeup), "send io cmd fail\n");
        do
        {
            DTOF_CHECK_RET(dtof_reg_burst_read(DTOF_ERROR_INFO_REG_ADDR, &temp, 1), "read reg failed\n");
            if (++try_count >= DTOF_MAX_RETRY_COUNT)
            {
                return DTOF_RET_ERROR;
            }
        } while (temp != DTOF_WAKEUP_FLAG);
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
DTOF_RET dtof_get_uuid(dtof_uint8_t *uuid_p, dtof_uint8_t len)
{
    DTOF_RET ret;
    ret = dtof_io_interaction(DTOF_CMD_GET_UUID, DTOF_CMD_NONE);
    if (ret != DTOF_RET_SUCCESS)
    {
        DTOF_LOG("file: %s, line: %d, send io cmd fail\n", __FILE__, __LINE__);
        return ret;
    }
    ret = dtof_reg_burst_read(DTOF_SAVE_RESULT_REG_ADDR, (dtof_uint16_t *)uuid_p, len / 2);
    if (ret != DTOF_RET_SUCCESS)
    {
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
DTOF_RET dtof_get_error_info(dtof_uint32_t *status)
{
    DTOF_RET ret;
    dtof_uint16_t reg_data;
    ret = dtof_io_interaction(DTOF_CMD_SYS_ERROR, DTOF_VAL_SYS_ERROR_GET);
    if (ret != DTOF_RET_SUCCESS)
    {
        DTOF_LOG("file: %s, line: %d, send io cmd fail\n", __FILE__, __LINE__);
        return ret;
    }
    ret = dtof_reg_burst_read(DTOF_ERROR_INFO_REG_ADDR, &reg_data, 1);
    if (ret != DTOF_RET_SUCCESS)
    {
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
DTOF_RET dtof_clear_error_info(void)
{
    DTOF_RET ret;
    ret = dtof_io_interaction(DTOF_CMD_SYS_ERROR, DTOF_VAL_SYS_ERROR_CLR);
    if (ret != DTOF_RET_SUCCESS)
    {
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
DTOF_RET dtof_clear_eye_safety_error_info(void)
{
    DTOF_RET ret;
    ret = dtof_io_interaction(DTOF_CMD_SYS_ERROR, DTOF_VAL_SYS_ERROR_EYE_SAFETY_CLR);
    if (ret != DTOF_RET_SUCCESS)
    {
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
DTOF_RET dtof_set_io_voltage(dtof_uint8_t value)
{
    DTOF_RET ret;
    ret = dtof_io_interaction(DTOF_CMD_SET_VCCIO, value);
    if (ret != DTOF_RET_SUCCESS)
    {
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
 */
DTOF_RET dtof_ram_code_burn(const dtof_uint16_t *ram_code, dtof_uint16_t len, dtof_uint8_t mode, dtof_uint16_t ram_offset)
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

    do
    {
        // update verison cmd
        ret = dtof_io_interaction(DTOF_CMD_SWITCH_VERSION, mode);
        if (ret != DTOF_RET_SUCCESS)
        {
            DTOF_LOG("file: %s, line: %d, send io cmd fail\n", __FILE__, __LINE__);
            return ret;
        }

        // wait inner mcu ready
        dtof_sleep_ms(DTOF_RAM_CODE_BURN_DELAY);

        // 烧写程序 iic / spi
        ret = dtof_reg_burst_write_burn(DTOF_READ_RAM_START_REG_ADDR, ram_code, len);
        if (ret != DTOF_RET_SUCCESS)
        {
            DTOF_LOG("file: %s, line: %d, reg write fail\n", __FILE__, __LINE__);
            return ret;
        }

        // wake up inner mcu
        ret = dtof_set_mcu_status(DTOF_MCU_STATE_WAKEUP);
        if (ret != DTOF_RET_SUCCESS)
        {
            DTOF_LOG("file: %s, line: %d, wake up mcu fail\n", __FILE__, __LINE__);
            return ret;
        }

        if (dtof_get_chip_config()->chip_id == DTOF_L3_CHIPID)
        {
            ret = dtof_reg_burst_read(DTOF_CHECK_CRC_REG_ADDR, &crc_result, 1);
            if (ret != DTOF_RET_SUCCESS)
            {
                DTOF_LOG("file: %s, line: %d, reg read fail\n", __FILE__, __LINE__);
                return ret;
            }

            // check crc result
            if ((crc_result & 0xff) == 0xAA)
            {
                ret = DTOF_RET_SUCCESS;
            }
            else
            {
                ret = crc_result & 0xff;
            }
        }

        try_time++;

    } while (try_time < 3 && ret != DTOF_RET_SUCCESS);

    return ret;
}

static DTOF_RET dtof_reset(dtof_uint16_t *chip_id, dtof_uint8_t *chip_uuid, dtof_uint32_t chip_uuid_len)
{
#define DTOF_INIT_DELAY 70
#define DTOF_BYPASS_DELAY 1
#define DTOF_WAIT_READY_DELAY 10
#define DTOF_INIT_FRAME_ID 1
    DTOF_RET ret = DTOF_RET_SUCCESS;
    dtof_uint16_t burn_try_count = 0;
    dtof_uint16_t frame_id;
    dtof_uint16_t jmpmode;
    dtof_uint16_t osc_ftrim;
    dtof_uint16_t osc_ftrim_reg;
    dtof_uint16_t reset_value = 0xffff;
    dtof_uint16_t bypass_value = 0x17b9;

    dtof_sleep_ms(DTOF_INIT_DELAY);

    // read uuid
    do
    {
        dtof_sleep_ms(DTOF_BYPASS_DELAY);
        ret = dtof_get_uuid(chip_uuid, chip_uuid_len);
        if (++burn_try_count > DTOF_MAX_RETRY_COUNT)
        {
            DTOF_CHECK_RET(DTOF_RET_FAILED, ("read chip uuid fail\n"));
            break;
        }
    } while (ret != DTOF_RET_SUCCESS);
    burn_try_count = 0;

    // bypass inner mcu
    DTOF_CHECK_RET(dtof_reg_burst_write(DTOF_IO_CTRL_REG_ADDR, &bypass_value, 1), "dtof bypass fail\n");

    // read chip id
    DTOF_CHECK_RET(dtof_reg_burst_read(DTOF_CHIP_ID_REG_ADDR, chip_id, 1), "reg read chip id fail\n");

    if (*chip_id == DTOF_A05_CHIPID)
    {
        // read jmpmode
        DTOF_CHECK_RET(dtof_reg_burst_read(DTOF_JMP_MODE_REG_ADDR, &jmpmode, 1), "reg read chip id fail\n");
        jmpmode &= ~(1 << 2);

        // read osc ftrim   
        uint16_t temp;
        dtof_uint16_t ram_start = 0x860;
        do
        {
            dtof_reg_burst_read(DTOF_DEBUG_STATUS_REG_ADDR, &temp, 1);
        } while ((temp & 0x7) != 1);

        dtof_reg_burst_read(DTOF_CONFIG_REG_ADDR, &temp, 1);
        temp &= ~(1 << 3);
        DTOF_CHECK_RET(dtof_reg_burst_write(DTOF_CONFIG_REG_ADDR, &temp, 1), "reg write config reg fail\n");

        DTOF_CHECK_RET(dtof_reg_burst_write(DTOF_WRITE_RAM_START_REG_ADDR, &ram_start, 1), "write ram start addr failed\n");
        DTOF_CHECK_RET(dtof_reg_burst_read(DTOF_READ_RAM_START_REG_ADDR, &osc_ftrim, 1), "read osc ftrim failed\n");
    }

    // reset chip
    DTOF_CHECK_RET(dtof_reg_burst_write(DTOF_RESET_REG_ADDR, &reset_value, 1), "chip reset fail\n");

    // wait chip ready
    dtof_sleep_ms(DTOF_INIT_DELAY);

    do
    {
        dtof_sleep_ms(DTOF_WAIT_READY_DELAY);
        DTOF_CHECK_WARN(dtof_reg_burst_read(DTOF_SAVE_RESULT_REG_ADDR, &frame_id, 1), "reg frame id fail\n");
        if (++burn_try_count > DTOF_MAX_RETRY_COUNT)
        {
            DTOF_CHECK_RET(DTOF_RET_FAILED, ("dtof init fail\n"));
            break;
        }
    } while ((frame_id < DTOF_INIT_FRAME_ID) && (frame_id != 0xffff));

    if (DTOF_A05_CHIPID == *chip_id)
    {
        DTOF_CHECK_RET(dtof_write_reg_running(DTOF_JMP_MODE_REG_ADDR, jmpmode), "reg write jmpmode fail\n");
        if (strncmp((const char *)chip_uuid, "N61V33", 6) == 0)
        {
            DTOF_CHECK_RET(dtof_read_reg_running(DTOF_TRIM_REG_ADDR, &osc_ftrim_reg), "read osc ftrim reg failed\n");
            osc_ftrim_reg &= 0xff00;
            osc_ftrim_reg |= osc_ftrim;
            DTOF_CHECK_RET(dtof_write_reg_running(DTOF_TRIM_REG_ADDR, osc_ftrim_reg), "write osc ftrim failed\n");
        }
    }

    DTOF_LOG("chip id = 0x%04x\n", *chip_id);
    return DTOF_RET_SUCCESS;
}

static DTOF_RET dtof_version_upgrade(const dtof_uint16_t *ram_code, dtof_uint16_t len, dtof_uint8_t mode, dtof_uint16_t ram_offset, dtof_uint16_t check_code)
{
#define DTOF_RAM_JUDGE_RES_DELAY 1
    DTOF_RET ret;
    dtof_uint16_t burn_end_flag = 0;
    dtof_uint16_t burn_try_count = 0;
    dtof_uint16_t burn_judge_count = 0;

    do
    {
        ret = dtof_ram_code_burn(ram_code, len, mode, ram_offset);
        if (!ret)
        {
            // 判断升级结束flag
            burn_judge_count = 0;
            do
            {
                DTOF_CHECK_RET(dtof_reg_burst_read(DTOF_CHECK_UPGRADE_REG_ADDR, &burn_end_flag, 1), "reg read fail\n");
                if (burn_end_flag == check_code)
                {
                    break;
                }
                dtof_sleep_ms(DTOF_RAM_JUDGE_RES_DELAY);
            } while (++burn_judge_count <= DTOF_MAX_RETRY_COUNT);
        }

        if (++burn_try_count >= DTOF_MAX_RETRY_COUNT)
        {
            DTOF_LOG_ERR("dtof version upgrade fail\n");
            return DTOF_RET_FAILED;
        }
    } while ((burn_try_count != DTOF_MAX_RETRY_COUNT) && (burn_end_flag != check_code));

    return ret;
}

static DTOF_RET dtof_select_working_mode(void)
{
#define DTOF_WAIT_VERSION_UPGRADE_DELAY 10
#define DTOF_PRE_CONFIG_END_FLAG 0xA1ED
#define DTOF_DISTANCE_INIT_END_FLAG 0xA2ED
#define DTOF_DISTANCE_MODE_END_FLAG 0xA3ED
    const dtof_uint16_t *ram_code_p = NULL;
    dtof_uint16_t ram_code_len;

    if (dtof_get_chip_config()->chip_id == DTOF_L3_CHIPID)
    {
        ram_code_p = ram_code_all[DTOF_L3_PRE_CONFIG].ram_code_ptr;
        ram_code_len = ram_code_all[DTOF_L3_PRE_CONFIG].ram_code_size;
        DTOF_CHECK_RET(dtof_version_upgrade(ram_code_p, ram_code_len, DTOF_SWB_TYPE_FROM_RAM_FIXADDR, 0, DTOF_PRE_CONFIG_END_FLAG), "patch1 upgrade fail\n");

        dtof_sleep_ms(DTOF_WAIT_VERSION_UPGRADE_DELAY);

        ram_code_p = ram_code_all[DTOF_L3_DISTANCE_INIT].ram_code_ptr;
        ram_code_len = ram_code_all[DTOF_L3_DISTANCE_INIT].ram_code_size;
        DTOF_CHECK_RET(dtof_version_upgrade(ram_code_p, ram_code_len, DTOF_SWB_TYPE_FROM_RAM_FIXADDR, 0, DTOF_DISTANCE_INIT_END_FLAG), "patch2 upgrade fail\n");

        dtof_sleep_ms(DTOF_WAIT_VERSION_UPGRADE_DELAY);

        ram_code_p = ram_code_all[DTOF_L3_DISTANCE_MODE].ram_code_ptr;
        ram_code_len = ram_code_all[DTOF_L3_DISTANCE_MODE].ram_code_size;
        DTOF_CHECK_RET(dtof_version_upgrade(ram_code_p, ram_code_len, DTOF_SWB_TYPE_FROM_RAM_FIXADDR, 0, DTOF_DISTANCE_MODE_END_FLAG), "patch3 upgrade fail\n");
    }
    else if (dtof_get_chip_config()->chip_id == DTOF_A05_CHIPID)
    {
        ram_code_p = ram_code_all[DTOF_A05_DISTANCE_INIT].ram_code_ptr;
        ram_code_len = ram_code_all[DTOF_A05_DISTANCE_INIT].ram_code_size;
        DTOF_CHECK_RET(dtof_version_upgrade(ram_code_p, ram_code_len, DTOF_SWB_TYPE_FROM_RAM_FIXADDR, 0, DTOF_DISTANCE_INIT_END_FLAG), "patch2 upgrade fail\n");

        dtof_sleep_ms(DTOF_WAIT_VERSION_UPGRADE_DELAY);

        ram_code_p = ram_code_all[DTOF_A05_DISTANCE_MODE].ram_code_ptr;
        ram_code_len = ram_code_all[DTOF_A05_DISTANCE_MODE].ram_code_size;
        DTOF_CHECK_RET(dtof_version_upgrade(ram_code_p, ram_code_len, DTOF_SWB_TYPE_FROM_RAM_FIXADDR, 0, DTOF_DISTANCE_MODE_END_FLAG), "patch3 upgrade fail\n");
    }
    else
    {
        DTOF_LOG("unknown chip id\n");
        return DTOF_RET_FAILED;
    }

    return DTOF_RET_SUCCESS;
}

DTOF_RET dtof_set_ft_data(dtof_uint16_t *dtof_calibrate_data_ft_p)
{
#define DTOF_FT_DATA_OFFSET 40
#define DTOF_FT_DATA_START 0x2000
#define DTOF_FT_DATA_LEN (sizeof(dtof_ft_data_t) / sizeof(dtof_uint16_t))
    dtof_uint16_t dtof_ft_data_start;
    dtof_uint16_t dtof_ft_data_inner[DTOF_FT_DATA_LEN];
    dtof_ft_data_t *dtof_ft_data_inner_p = (dtof_ft_data_t *)dtof_ft_data_inner;
    dtof_ft_data_t *dtof_ft_data_p = (dtof_ft_data_t *)dtof_calibrate_data_ft_p;

    if (dtof_get_chip_config()->chip_id == DTOF_L3_CHIPID)
    {
        dtof_ft_data_start = ram_code_all[DTOF_L3_DISTANCE_MODE].ram_code_size + DTOF_FT_DATA_START - DTOF_FT_DATA_OFFSET;
    }
    else if (dtof_get_chip_config()->chip_id == DTOF_A05_CHIPID)
    {
        dtof_ft_data_start = ram_code_all[DTOF_A05_DISTANCE_MODE].ram_code_size + DTOF_FT_DATA_START - DTOF_FT_DATA_OFFSET;
    }
    else
    {
        DTOF_LOG("unknown chip id\n");
        return DTOF_RET_FAILED;
    }

    DTOF_CHECK_RET(dtof_set_mcu_status_ram(DTOF_MCU_STATE_SLEEP_DIRECT), "set mcu sleep failed\n");

    DTOF_CHECK_RET(dtof_reg_burst_write(DTOF_WRITE_RAM_START_REG_ADDR, &dtof_ft_data_start, 1), "write ram start addr failed\n");
    DTOF_CHECK_RET(dtof_reg_burst_read(DTOF_READ_RAM_START_REG_ADDR, dtof_ft_data_inner, DTOF_FT_DATA_LEN), "read ft data failed\n");

    // 填充ft数据
#ifdef DTOF_FT_CALIBRATE_BINOFFSET
    dtof_ft_data_inner_p->bin_offset = dtof_ft_data_p->bin_offset;
#endif
#ifdef DTOF_FT_CALIBRATE_REFSPAD
    dtof_ft_data_inner_p->ref_spad = dtof_ft_data_p->ref_spad;
#endif
#ifdef DTOF_FT_CALIBRATE_CG
    dtof_memcpy(dtof_ft_data_inner_p->cg_data, dtof_ft_data_p->cg_data, sizeof(dtof_ft_data_inner_p->cg_data));
    for (int i = 0; i < CROSS_TALK_OTP_NUM; i++)
    {
        dtof_ft_data_inner_p->cg_data[i] = dtof_ft_data_p->cg_data[i];
    }
#endif
#ifdef DTOF_FT_CALIBRATE_B
    dtof_ft_data_inner_p->distance_b = dtof_ft_data_p->distance_b;
    dtof_ft_data_inner_p->distance_k = dtof_ft_data_p->distance_k;
#endif

    DTOF_CHECK_RET(dtof_reg_burst_write(DTOF_WRITE_RAM_START_REG_ADDR, &dtof_ft_data_start, 1), "write ram start addr failed\n");
    DTOF_CHECK_RET(dtof_reg_burst_write(DTOF_READ_RAM_START_REG_ADDR, dtof_ft_data_inner, DTOF_FT_DATA_LEN), "write ft data failed\n");

    DTOF_CHECK_RET(dtof_set_mcu_status_ram(DTOF_MCU_STATE_WAKEUP), "wakeup mcu failed\n");

    DTOF_CHECK_RET(dtof_reload_ft_data(), "reload ft data failed\n");

#define DTOF_WAIT_RELOAD_FT_DATA 5
    dtof_sleep_ms(DTOF_WAIT_RELOAD_FT_DATA);

    return DTOF_RET_SUCCESS;
}



DTOF_RET dtof_sensor_init(void)
{
    dtof_uint16_t chip_id;
    dtof_uint8_t chip_uuid[DTOF_UUID_LENGTH];
    dtof_ft_data_t ft_data;
    dtof_bool_t is_legal_ft_data = DTOF_FALSE;

//     dtof_ft_data_t ft_data = {
//     .cg_data = {0x1234, 0x5678, 0x9ABC},
//     .distance_k = 0x0001,       // 给distance_k赋值
//     .distance_b = 0x0002,       // 给distance_b赋值
//     .ref_spad = 0x0003,         // 给ref_spad赋值
//     .bin_offset = 0x0004        // 给bin_offset赋值
// };

    if (dtof_device_info.chip_is_init == DTOF_FALSE)
    {
        printf("10");
        DTOF_CHECK_RET(dtof_reset(&chip_id, chip_uuid, DTOF_UUID_LENGTH), "dtof reset failed\n");
        DTOF_CHECK_RET(dtof_find_chip_config(chip_id, chip_uuid, DTOF_UUID_LENGTH), "find chip config failed\n");
        DTOF_CHECK_RET(dtof_select_working_mode(), "dtof select working mode failed\n");
        dtof_device_info.chip_is_init = DTOF_TRUE;
    }

  DTOF_CHECK_RET(dtof_get_ft_data_from_flash((dtof_uint16_t *)&ft_data, sizeof(dtof_ft_data_t) / sizeof(dtof_uint16_t), &is_legal_ft_data), "get ft data from flash failed\n");

    if (is_legal_ft_data == DTOF_TRUE)
    {printf("11");
        DTOF_CHECK_RET(dtof_set_ft_data((dtof_uint16_t *)&ft_data), "set ft data failed\n");
    }

    return DTOF_RET_SUCCESS;
}

/**
 * @brief This function is used to start the distance measurement.
 *
 * It will set the mcu status to sleep first, then write the start flag to the frame control register,
 * and finally set the mcu status to wakeup.
 *
 * @return DTOF_RET_SUCCESS if the start is successful, DTOF_RET_FAILED otherwise.
 */
DTOF_RET dtof_start_distance_measure(void)
{
    DTOF_CHECK_RET(dtof_write_reg_running(DTOF_FRAME_CONTROL_REG, DTOF_START_DISATNCE_MODE), "dtof start failed\n");
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

DTOF_RET dtof_stop_distance_measure(void)
{
    DTOF_CHECK_RET(dtof_write_reg_running(DTOF_FRAME_CONTROL_REG, DTOF_STOP_DISATNCE_MODE), "dtof start failed\n");
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

DTOF_RET dtof_quit_distance_measure(void)
{
    DTOF_CHECK_RET(dtof_write_reg_running(DTOF_FRAME_CONTROL_REG, DTOF_QUIT_DISATNCE_MODE), "dtof start failed\n");
    return DTOF_RET_SUCCESS;
}

/**
 * @brief This function is used to start do ft calibrate.
 * @return DTOF_RET_SUCCESS if the quit is successful, DTOF_RET_FAILED otherwise.
 */
DTOF_RET dtof_start_ft_calibrate(void)
{
    DTOF_CHECK_RET(dtof_write_reg_running(DTOF_FRAME_CONTROL_REG, DTOF_FT_CAL_START_MODE), "dtof start failed\n");
    return DTOF_RET_SUCCESS;
}

DTOF_RET dtof_reload_ft_data(void)
{
    DTOF_CHECK_RET(dtof_write_reg_running(DTOF_FRAME_CONTROL_REG, DTOF_RELOAD_FT_DATA_MODE), "dtof reload ft data failed\n");
    return DTOF_RET_SUCCESS;
}

const char *dtof_get_sdk_version(void)
{
    return DTOF_VERSION_STRING;
}

dtof_uint16_t dtof_get_chip_version(void)
{
    dtof_uint16_t sdk_version;
    if (dtof_get_chip_config()->chip_id == DTOF_L3_CHIPID)
    {
        sdk_version = ram_code_all[DTOF_L3_DISTANCE_MODE].ram_code_ptr[ram_code_all[DTOF_L3_DISTANCE_MODE].ram_code_size - 2];
    }
    else if (dtof_get_chip_config()->chip_id == DTOF_A05_CHIPID)
    {
        sdk_version = ram_code_all[DTOF_A05_DISTANCE_MODE].ram_code_ptr[ram_code_all[DTOF_A05_DISTANCE_MODE].ram_code_size - 2];
    }
    else
    {
        DTOF_LOG("unknown chip id\n");
        sdk_version = DTOF_VERSION_UNKNOWN;
    }

    return sdk_version;
}
