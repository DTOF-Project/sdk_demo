/*
 * dtof_io.c
 *
 *  Created on: 2024/8/5
 *      Author: liuzihao
 */
#include "sdk/inc/dtof_common.h"
#include "sdk/inc/dev/dtof_reg.h"
#include "sdk/inc/dtof_log.h"
#include "inc/dtof_driver.h"
#include "sdk/inc/dtof_api.h"
#include "inc/dtof_libc.h"
#include "inc/dev/dtof_dev_api.h"
#include "sdk/inc/dtof_global_config.h"

DTOF_RET dtof_bypass_inner_mcu_external(void)
{
    DTOF_RET ret;
    dtof_uint16_t bypass_val = 0x17B9;
    ret = dtof_reg_burst_write(device_id, DTOF_REG5, &bypass_val, 1);
    if(ret != DTOF_RET_SUCCESS){
        DTOF_LOG("file: %s, line: %d, reg write fail\n", __FILE__, __LINE__);
        return ret;
    }
    return ret;
}

DTOF_RET dtof_wakeup_inner_mcu_external(void)
{
    DTOF_RET ret;
    dtof_uint16_t wakeup_val = 0;
    ret = dtof_reg_burst_write(device_id, DTOF_REG5, &wakeup_val, 1);
    if(ret != DTOF_RET_SUCCESS){
        DTOF_LOG("file: %s, line: %d, reg write fail\n", __FILE__, __LINE__);
        return ret;
    }
    return ret;
}

static dtof_uint32_t g_read_times = 0;
static dtof_bool_t g_read_flag = DTOF_FALSE;
static dtof_uint16_t fifo_temp[93]; // 有效数据31 + 31 + 11 = 73

void dtof_init_mul_res_temp(void)
{
    dtof_memset(fifo_temp, 0, sizeof(fifo_temp));
}

dtof_uint32_t dtof_get_mul_read_times(void)
{
    return g_read_times;
}

void dtof_set_mul_read_times(dtof_uint32_t mul_read_times)
{
    g_read_times = mul_read_times;
}

dtof_bool_t dtof_get_mul_read_flag(void)
{
    return g_read_flag;
}

void dtof_set_mul_read_flag(dtof_bool_t mul_read_flag)
{
    g_read_flag = mul_read_flag;
}

#ifdef DTOF_L3
DTOF_RET dtof_get_multiple_result(dtof_uint16_t *mul_result_p)
{
    DTOF_RET ret = DTOF_RET_ERROR;

    if(DTOF_TRUE == dtof_get_interrupt_flag())
    {
        if(g_read_flag == DTOF_FALSE)
        {
            // 发现本帧和前一帧的fifo11 ~ fifo30相等, 判断本帧为一轮多点的最后一帧
            dtof_reg_burst_read(device_id, DTOF_REG80, fifo_temp + g_read_times * 31, 31);

            g_read_times++;
            if(g_read_times == 3){
                // 检测是否为合法帧, 不是合法帧的话把第一次读的废弃
                if((memcmp(&fifo_temp[42], &fifo_temp[73], 20 * sizeof(uint16_t)) != 0) || (fifo_temp[0] != 1))
                {
//                    memcpy(fifo_temp, &fifo_temp[31], 62 * sizeof(uint16_t));
                    memmove(fifo_temp, &fifo_temp[31], 62 * sizeof(uint16_t));

                    g_read_times = 2;
                }
                else
                {
                    g_read_times = 0;
                    g_read_flag = DTOF_TRUE;

                    memcpy(mul_result_p, fifo_temp, 73*2);
                    ret = DTOF_RET_SUCCESS;
                }
            }
        }
        else
        {
            // 找到第一个合法帧后, 不需要再判断了, 每三帧一个合法帧
            dtof_reg_burst_read(device_id, 0x50, fifo_temp + g_read_times * 31, 31);
            g_read_times++;
            if(g_read_times == 3){
                g_read_times = 0;

                memcpy(mul_result_p, fifo_temp, 73*2);

                ret = DTOF_RET_SUCCESS;
            }
        }

        dtof_set_interrupt_flag(DTOF_FALSE);
    }
		return ret;
}

#elif defined(DTOF_A05)

DTOF_RET dtof_get_multiple_result(dtof_uint16_t *mul_result_p)
{
    DTOF_RET ret = DTOF_RET_ERROR;

    if(DTOF_TRUE == dtof_get_interrupt_flag())
    {
        if(g_read_flag == DTOF_FALSE)
        {
            // 发现本帧和前一帧的fifo11 ~ fifo30相等, 判断本帧为一轮多点的最后一帧
            dtof_reg_burst_read(device_id, DTOF_REG80, fifo_temp + g_read_times * 31, 31);

            g_read_times++;
            if(g_read_times == 3){
                // 检测是否为合法帧, 不是合法帧的话把第一次读的废弃
                if((memcmp(&fifo_temp[42], &fifo_temp[73], 20 * sizeof(uint16_t)) != 0) || (fifo_temp[0] != 1))
                {
                    memcpy(fifo_temp, &fifo_temp[31], 62 * sizeof(uint16_t));
                    g_read_times = 2;
                }
                else
                {
                    g_read_times = 0;
                    g_read_flag = DTOF_TRUE;

                    memcpy(mul_result_p, fifo_temp, 73*2);
                    ret = DTOF_RET_SUCCESS;
                }
            }
        }
        else
        {
            // 找到第一个合法帧后, 不需要再判断了, 每三帧一个合法帧
            dtof_reg_burst_read(device_id, 0x50, fifo_temp + g_read_times * 31, 31);
            g_read_times++;
            if(g_read_times == 3){
                g_read_times = 0;

                memcpy(mul_result_p, fifo_temp, 73*2);

                ret = DTOF_RET_SUCCESS;
            }
        }

        dtof_set_interrupt_flag(DTOF_FALSE);
    }
		return ret;
}

#endif

DTOF_RET dtof_switch_mode(dtof_uint16_t mode, dtof_uint16_t* reg_config_p, dtof_uint16_t len)
{
    DTOF_RET ret;

    switch(mode)
    {
        case DTOF_MODE_INFRARED:
        case DTOF_MODE_MUL_CP_FLAG:
        case DTOF_MODE_MUL_POS_FLAG:
        case DTOF_MODE_MUL_POS_ALGO_FLAG:
        {
            ret = dtof_io_interaction(device_id, DTOF_SWITCH_MODE_CMD, mode);
            if(ret != DTOF_RET_SUCCESS){
                DTOF_LOG("file: %s, line: %d, send io cmd fail\n", __FILE__, __LINE__);
            }
            return ret;
        }
        case DTOF_MODE_SMOKE:
        case DTOF_MODE_0_2_HZ_ATN_DISTANCE:
        case DTOF_MODE_1_HZ_ATN_DISTANCE:
        case DTOF_MODE_30_HZ_DISTANCE:
        case DTOF_MODE_20_HZ_DISTANCE:
        case DTOF_MODE_10_HZ_DISTANCE:
        {
            dtof_uint16_t temp;
            ret = dtof_io_interaction(device_id, DTOF_SWITCH_MODE_CMD, DTOF_SWITCH_MODE_NORMAL);
            if(ret != DTOF_RET_SUCCESS){
                DTOF_LOG("file: %s, line: %d, send io cmd fail\n", __FILE__, __LINE__);
                return ret;
            }

            // wait bypass TODO: timeout
            do{
                ret = dtof_reg_burst_read(device_id, DTOF_REG0, &temp, 1);
                if(ret != DTOF_RET_SUCCESS){
                    DTOF_LOG("file: %s, line: %d, reg read fail\n", __FILE__, __LINE__);
                    return ret;
                }
            }while(temp != DTOF_CHIP_ID);

            ret = dtof_reg_burst_write(device_id, 0xff, reg_config_p, len);
            if(ret != DTOF_RET_SUCCESS){
                DTOF_LOG("file: %s, line: %d, reg write fail\n", __FILE__, __LINE__);
                return ret;
            }

            ret = dtof_set_mcu_status(DTOF_MCU_STATE_WAKEUP);
            if(ret != DTOF_RET_SUCCESS){
                DTOF_LOG("file: %s, line: %d, wake up mcu fail\n", __FILE__, __LINE__);
                return ret;
            }
            break;
        }
        default:
        {
            DTOF_LOG("file: %s, line: %d, switch invalid mode\n", __FILE__, __LINE__);
            return DTOF_RET_ERROR;
        }
    }

    return ret;
}

DTOF_RET dtof_reset_and_close_wdt(void)
{
    DTOF_RET ret;
    ret = dtof_io_interaction(device_id, DTOF_CMD_RESET_AND_CLOSE_WDT, DTOF_CMD_NONE);
    if(ret != DTOF_RET_SUCCESS){
        DTOF_LOG("file: %s, line: %d, send io cmd fail\n", __FILE__, __LINE__);
        return ret;
    }
    return ret;
}

/**
 * @brief Switch to infrared mode and check
 * @param void
 * @return DTOF_RET_SUCCESS or DTOF_RET_ERROR
 * @note This function interacts with the I/O to switch the device mode to infrared.
 */
DTOF_RET dtof_switch_infrared_mode(void)
{
    DTOF_RET ret;
    dtof_uint16_t temp;
    ret = dtof_io_interaction(device_id, DTOF_INFRARED_MODE, 0X0);
    if(ret != DTOF_RET_SUCCESS){
        DTOF_LOG("file: %s, line: %d, send io cmd fail\n", __FILE__, __LINE__);
        return ret;
    }

    ret = dtof_set_mcu_status(DTOF_MCU_STATE_SLEEP_DIRECT);
    if(ret != DTOF_RET_SUCCESS){
        DTOF_LOG("file: %s, line: %d, mcu sleep fail\n", __FILE__, __LINE__);
        return ret;
    }

    // external config DRV_PUMP_EN = 1 (reg242 bit[8])
    ret = dtof_reg_burst_read(device_id, DTOF_REG242, &temp, 1);
    if(ret != DTOF_RET_SUCCESS){
        DTOF_LOG("file: %s, line: %d, reg read fail\n", __FILE__, __LINE__);
        return ret;
    }

    temp = temp | 0x100;

    ret = dtof_reg_burst_write(device_id, DTOF_REG242, &temp, 1);
    if(ret != DTOF_RET_SUCCESS){
        DTOF_LOG("file: %s, line: %d, reg write fail\n", __FILE__, __LINE__);
        return ret;
    }

    // clkRat write 3 (reg 130 bit[0,1] = 3)
    ret = dtof_reg_burst_read(device_id, DTOF_REG130, &temp, 1);
    if(ret != DTOF_RET_SUCCESS){
        DTOF_LOG("file: %s, line: %d, reg read fail\n", __FILE__, __LINE__);
        return ret;
    }

    temp = temp | 0x3;

    ret = dtof_reg_burst_write(device_id, DTOF_REG130, &temp, 1);
    if(ret != DTOF_RET_SUCCESS){
        DTOF_LOG("file: %s, line: %d, reg write fail\n", __FILE__, __LINE__);
        return ret;
    }

    ret = dtof_set_mcu_status(DTOF_MCU_STATE_WAKEUP);
    if(ret != DTOF_RET_SUCCESS){
        DTOF_LOG("file: %s, line: %d, wake up mcu fail\n", __FILE__, __LINE__);
        return ret;
    }
    return ret;
}



/**
 * @brief Start sending infrared signal
 * @param void
 * @return DTOF_RET_SUCCESS or DTOF_RET_ERROR
 * @note This function prepares the device to send an infrared signal by
 *       setting the necessary command and parameters through I/O interaction.
 */
DTOF_RET dtof_start_send_infrared(void)
{
    DTOF_RET ret;
    // ready to send data,  cmd 7 value 1
    ret = dtof_io_interaction(device_id, DTOF_INFRARED_CMD_REPEAT, 0X1);
    if(ret != DTOF_RET_SUCCESS){
        DTOF_LOG("file: %s, line: %d, send io cmd fail\n", __FILE__, __LINE__);
        return ret;
    }
    return ret;
}



/**
 * @brief Send infrared info
 * @param value The value to be sent.
 * @param mode The mode to be used to send the value. has valid mode and repeat mode
 * @return DTOF_RET_SUCCESS or DTOF_RET_ERROR
 * @note This function sends the cmd to ctl device send infrared info
 */
DTOF_RET dtof_send_infrared_info(dtof_uint8_t value, dtof_uint32_t mode)
{
    #define DTOF_INFRARED_REPEAT_FLAG 0X01
    #define DTOF_INFRARED_VALUE_FLAG 0X00
    DTOF_RET ret;
    if (mode == DTOF_INFRARED_REPEAT_FLAG){
        ret = dtof_io_interaction(device_id, DTOF_INFRARED_CMD_REPEAT, 0X0);
        if(ret != DTOF_RET_SUCCESS){
            DTOF_LOG("file: %s, line: %d, send io cmd fail\n", __FILE__, __LINE__);
            return ret;
        }
    }else if(mode == DTOF_INFRARED_VALUE_FLAG){
        ret = dtof_io_interaction(device_id, DTOF_INFRARED_CMD_VALID, value);
        if(ret != DTOF_RET_SUCCESS){
            DTOF_LOG("file: %s, line: %d, send io cmd fail\n", __FILE__, __LINE__);
            return ret;
        }
    }else{
        DTOF_LOG("file: %s, line: %d, invalid infrared mode\n", __FILE__, __LINE__);
        ret = DTOF_RET_ERROR;
    }

    return ret;
}




/**
 * @brief Quit infrared mode
 * @param void
 * @return DTOF_RET_SUCCESS or DTOF_RET_ERROR
 * @note This function is used to quit infrared mode. It sends the
 *       necessary command to the device to stop infrared mode.
 */
DTOF_RET dtof_quit_infrared_mode(void)
{
    DTOF_RET ret;
    dtof_uint16_t temp = 1;

    ret = dtof_set_mcu_status(DTOF_MCU_STATE_SLEEP_DIRECT);
    if(ret != DTOF_RET_SUCCESS){
        DTOF_LOG("file: %s, line: %d, mcu sleep fail\n", __FILE__, __LINE__);
        return ret;
    }

    // recover reg 130
    ret = dtof_reg_burst_write(device_id, DTOF_REG130, &temp, 1);
    if(ret != DTOF_RET_SUCCESS){
        DTOF_LOG("file: %s, line: %d, reg write fail\n", __FILE__, __LINE__);
        return ret;
    }

    ret = dtof_set_mcu_status(DTOF_MCU_STATE_WAKEUP);
    if(ret != DTOF_RET_SUCCESS){
        DTOF_LOG("file: %s, line: %d, wake up mcu fail\n", __FILE__, __LINE__);
        return ret;
    }

    ret = dtof_io_interaction(device_id, DTOF_INFRARED_CMD_QUIT, 0X0);
    if(ret != DTOF_RET_SUCCESS){
        DTOF_LOG("file: %s, line: %d, send io cmd fail\n", __FILE__, __LINE__);
        return ret;
    }

    return ret;
}

/**
 * @brief write one byte to OTP
 * @param value the value to be written to OTP. The low byte is the address and the high byte is the value.
 * @param time the time to delay after writing OTP. The unit is 10us.
 * @return DTOF_RET_SUCCESS or DTOF_FAIL
 * @note This function writes one byte to OTP. The value should be written as (offset_addr + index) + (*(out_buf + index) << 8).
 *       The address should be written in the low byte and the value should be written in the high byte. The function will
 *       delay for the specified time after writing OTP.
 */
DTOF_RET dtof_write_otp_byte(dtof_uint16_t value, dtof_uint16_t time){
    // TODO: 这里为什么不extern usleep 这个接口应该不会提供给客户？
   extern int usleep(int micro_seconds);
    // value = (offset_addr + index) + (*(out_buf + index) << 8); low byte is addr, high byte is value
    #define PWE_ENABLE 0x0033                // after otp, mvpp, pprog enable and write PA PDIN
    #define PWE_DISABLE 0x0031               // first disable pwe after write otp
    DTOF_RET ret;
    dtof_uint16_t temp, value_temp;
    value_temp = value;

    // dtof_uint16_t addr, data;
    // addr = (value >> 8) & 0xff;
    // data = value & 0xff;
    // ret |= dtof_bypass_inner_mcu_external();
    ret = dtof_reg_burst_write(device_id, DTOF_REG252, &value_temp, 1);
    temp = PWE_ENABLE;
    // OTP_CONTROL_REG_ADDRESS
    ret |= dtof_reg_burst_write(device_id, OTP_CONTROL_REG_ADDRESS, &temp, 1);
    temp = PWE_DISABLE;
    while (time > 0){
        time--;
        usleep(10);
    }
    //deley one read
    // ret |= dtof_reg_burst_read(device_id, DTOF_REG0, &delay_temp, 1);
    ret |= dtof_reg_burst_write(device_id, OTP_CONTROL_REG_ADDRESS, &temp, 1);
    return ret;
}

/**
 * @brief switch run mode for multiple mode
 * @param mode cp/8*8
 * @return DTOF_RET_SUCCESS or DTOF_FAIL
 * @note
 */
DTOF_RET dtof_switch_multiple_mode(dtof_uint32_t mode)
{
    DTOF_RET ret;
    ret = dtof_io_interaction(device_id, DTOF_SWITCH_MUTILPLE_MODE_CMD, mode);
    if(ret != DTOF_RET_SUCCESS){
        DTOF_LOG("file: %s, line: %d, send io cmd fail\n", __FILE__, __LINE__);
        return ret;
    }
    return ret;
}

DTOF_RET dtof_switch_multiple_mode_another(dtof_uint32_t mode)
{
    DTOF_RET ret;
    ret = dtof_io_interaction(device_id, DTOF_SWITCH_MODE_CMD, mode);
    if(ret != DTOF_RET_SUCCESS){
        DTOF_LOG("file: %s, line: %d, send io cmd fail\n", __FILE__, __LINE__);
        return ret;
    }
    return ret;
}

/// @brief manual control run/stop calculate process
/// @param mode run/stop process input zero means run, else means stop
/// @return DTOF_RET_SUCCESS or DTOF_RET_ERROR
DTOF_RET dtof_manual_control(dtof_uint32_t value)
{
    DTOF_RET ret;
    ret = dtof_io_interaction(device_id, DTOF_CMD_MANUAL_CONTROL, value);
    if(ret != DTOF_RET_SUCCESS){
        DTOF_LOG("file: %s, line: %d, send io cmd fail\n", __FILE__, __LINE__);
        return ret;
    }
    return ret;
}

/// @brief change report result mode
/// @param report_mode trigger/pull up/pull down/do nothing
/// @return DTOF_RET_SUCCESS or DTOF_RET_ERROR
DTOF_RET dtof_change_report_result_mode(dtof_uint32_t report_mode)
{
    DTOF_RET ret;
    ret = dtof_io_interaction(device_id, DTOF_CMD_CHANGE_REPORT_RESULT_MODE, report_mode);
    if(ret != DTOF_RET_SUCCESS){
        DTOF_LOG("file: %s, line: %d, send io cmd fail\n", __FILE__, __LINE__);
        return ret;
    }
    return ret;
}

/// @brief change fsm status
/// @param fsm_status run once/free run/free run without fdly/hold run
/// @return DTOF_RET_SUCCESS or DTOF_RET_ERROR
DTOF_RET dtof_change_fsm_status(dtof_uint32_t fsm_status)
{
    DTOF_RET ret;
    ret = dtof_io_interaction(device_id, DTOF_CMD_CHANGE_FSM_STATUS, fsm_status);
    if(ret != DTOF_RET_SUCCESS){
        DTOF_LOG("file: %s, line: %d, send io cmd fail\n", __FILE__, __LINE__);
        return ret;
    }
    return ret;
}

DTOF_RET dtof_set_smoke_ignore_frame(dtof_uint16_t ignore_frame)
{
    DTOF_RET ret;
    ret = dtof_io_interaction(device_id, DTOF_CMD_SET_SMOKE_IGNORE_FRAME, ignore_frame);
    if(ret != DTOF_RET_SUCCESS){
        DTOF_LOG("file: %s, line: %d, send io cmd fail\n", __FILE__, __LINE__);
        return ret;
    }
    return ret;
}

/// @brief stop accumulate mode (30hz distance)
/// @param void
/// @return DTOF_RET_SUCCESS or DTOF_RET_ERROR
DTOF_RET dtof_stop_accumulate_mode(void)
{
    DTOF_RET ret;
    ret = dtof_io_interaction(device_id, DTOF_CMD_STOP_ACCUMULATE_MODE, 0);
    if(ret != DTOF_RET_SUCCESS){
        DTOF_LOG("file: %s, line: %d, send io cmd fail\n", __FILE__, __LINE__);
        return ret;
    }
    return ret;
}

DTOF_RET dtof_debug(dtof_uint32_t mode)
{
    DTOF_RET ret;
    ret = dtof_io_interaction(device_id, DTOF_CMD_DEBUG, mode);
    if(ret != DTOF_RET_SUCCESS){
        DTOF_LOG("file: %s, line: %d, send io cmd fail\n", __FILE__, __LINE__);
        return ret;
    }
    return ret;
}

/// @brief get smoke result
/// @param result_p smoke result
/// @return DTOF_RET_SUCCESS or DTOF_RET_ERROR
DTOF_RET dtof_get_smoke_result(dtof_smoke_result_t *result)
{
    DTOF_RET ret;
    dtof_uint16_t smoke_result[sizeof(dtof_smoke_result_t) / sizeof(dtof_uint16_t)];

    ret = dtof_reg_burst_read(device_id, DTOF_REG80, smoke_result, sizeof(dtof_smoke_result_t) / sizeof(dtof_uint16_t));
    if(ret != DTOF_RET_SUCCESS){
        DTOF_LOG("file: %s, line: %d, reg read fail\n", __FILE__, __LINE__);
        return ret;
    }

    result->temp_result = ((dtof_uint64_t)smoke_result[3] << 48) |
                          ((dtof_uint64_t)smoke_result[2] << 32) |
                          ((dtof_uint64_t)smoke_result[1] << 16) |
                          ((dtof_uint64_t)smoke_result[0]);

    result->result = ((dtof_uint32_t)smoke_result[5] << 16) |
                     ((dtof_uint32_t)smoke_result[4]);

    return ret;
}

/**
 * @brief Calculates the IIC bus rate according to the given IIC speed.
 *
 * rate = (osccnt/refcnt - [(125/0.400)*(0.400/x)]) / [(125/0.400)*(0.400/x)]
 *
 * @param iic_speed the IIC bus speed in MHZ, should be above 0.125 M ,if less than 0.125 M, osc_cnt will plus 4096*4
 * @return the calculated IIC bus rate in MHZ
 */
dtof_real32_t dtof_iic_calculate(dtof_real32_t iic_speed)
{
    // TODO struct rate,osc_cnt,ref_cnt,     移到common
    #define IIC_SPEED_400K (0.400)
    #define IIC_PARAM_125 (125.0)
    #define IIC_SPEED_125K (0.125)
    #define IIC_400K_STEP (IIC_PARAM_125/IIC_SPEED_400K)
    dtof_uint32_t osc_cnt = 0;
    dtof_uint32_t ref_cnt = 0;
    dtof_real32_t rate = 0;
    // eg: iic speed = 0.400 M
    // rate = ( osccnt/refcnt - [(125/0.400)*(0.400/x)] ) /  [(125/0.400)*(0.400/x)]
    if (iic_speed < IIC_SPEED_125K){
        // 当i2c 时钟在125K以及以下的时候，asic内部中间计数器会溢出。一笔写操作后，读出来osccnt值需要额外加4096*4。
        osc_cnt += 4096*4;
    }
    rate = ((osc_cnt / ref_cnt) - IIC_400K_STEP*(IIC_SPEED_400K/iic_speed)) / (IIC_400K_STEP*(IIC_SPEED_400K/iic_speed));
    return rate;
}

/**
 * @brief iic calculate return
 * @param iic_speed iic speed
 * @return DTOF_RET_SUCCESS or DTOF_RET_ERROR
 * @note iic calculate ret should be less than 0.0001
 */
DTOF_RET dtof_iic_calculate_ret(dtof_real32_t iic_speed){
    #define IIC_ACCURATE_10000 (10000)
    DTOF_RET ret = DTOF_RET_SUCCESS;
    dtof_real32_t rate = 0;
    dtof_uint64_t rate_uint = 0;
    rate = dtof_iic_calculate(iic_speed);
    rate_uint = (dtof_uint64_t)(rate * IIC_ACCURATE_10000);
    if (rate_uint > 0){
        ret = DTOF_RET_ERROR;
    }
    return ret;
}

// ram中每帧后输出dsp fifo
DTOF_RET dtof_ram_output_dsp_fifo(void)
{
    DTOF_RET ret;
    dtof_uint16_t ram_code[] = {0x07b7, 0x9300, 0xd783, 0x1107, 0x1171, 0x07c2, 0x83c1, 0xf713, 0x0077, 0x7713, 0x0ff7, 0x1023, 0x00e1, 0x5703, 0x0001, 0x1123, 0x00f1, 0x1793, 0x0107, 0x4585, 0x83c1, 0x0637, 0x9300, 0x4685, 0x8663, 0x02b7, 0x0001, 0x5783, 0x1106, 0x07c2, 0x83c1, 0xf713, 0x0077, 0x7713, 0x0ff7, 0x1023, 0x00e1, 0x5703, 0x0001, 0x1123, 0x00f1, 0x1793, 0x0107, 0x83c1, 0x9ee3, 0xfcd7, 0x2737, 0x9000, 0x2783, 0xfa47, 0x4689, 0xc394, 0xc3d4, 0xc794, 0x06b7, 0x9000, 0x4605, 0x8693, 0x08c6, 0xc7d0, 0xa823, 0x0007, 0xa223, 0x0207, 0xd394, 0x2423, 0xfac7, 0x0111, 0x8082, 0x0000, 0x7139, 0x858a, 0x4665, 0x0513, 0x4000, 0xde06, 0x1097, 0xf000, 0x80e7, 0xdd00, 0x850a, 0x4605, 0x45e5, 0x3097, 0xf000, 0x80e7, 0xa5e0, 0x50f2, 0x6121, 0x8082, 0x0000, 0x0000, 0x55aa, 0x005e};
    ret = dtof_ram_code_burn(ram_code, sizeof(ram_code)/sizeof(dtof_uint16_t), DTOF_SWB_TYPE_FROM_RAM_FIXADDR, 0);
    if(ret != DTOF_RET_SUCCESS){
        DTOF_LOG("file: %s, line: %d, ram code burn fail\n", __FILE__, __LINE__);
        return ret;
    }
    return ret;
}

// ram中每帧后输出dsp fifo, fix low power mode
DTOF_RET dtof_ram_output_dsp_fifo_low_power(void)
{
    DTOF_RET ret;
    // 可以成功的版本
    // dtof_uint16_t ram_code[] = {0x0113, 0xf341, 0x4501, 0xc586, 0xc3a6, 0x2419, 0x04b7, 0x9300, 0xd703, 0x0064, 0x4509, 0x0742, 0x8341, 0x87ba, 0x9bc1, 0x1023, 0x00e1, 0xe793, 0x0037, 0x0023, 0x00f1, 0x5783, 0x0001, 0x9323, 0x00f4, 0x1097, 0xf000, 0x80e7, 0x8120, 0x0793, 0x0200, 0x9b23, 0x1ef4, 0x4551, 0x1097, 0xf000, 0x80e7, 0x6b00, 0x6785, 0x8793, 0x8007, 0x9e23, 0x1ef4, 0x01d0, 0x00dc, 0x06b7, 0x9300, 0x0001, 0xd703, 0x1fe6, 0x0785, 0x8fa3, 0xfee7, 0x9be3, 0xfec7, 0x9e23, 0x1e06, 0x9b23, 0x1e06, 0x4501, 0x0097, 0xf000, 0x80e7, 0x7cc0, 0x5783, 0x0001, 0x06b7, 0x9300, 0xe793, 0x0087, 0x1023, 0x00f1, 0x07c2, 0x83c1, 0x9323, 0x00f6, 0x27b7, 0x9000, 0x8793, 0xf847, 0xd703, 0x1e46, 0x579c, 0x1023, 0x00e1, 0xf793, 0x0407, 0x9f63, 0x1207, 0x4783, 0x05b1, 0x4683, 0x0561, 0x4703, 0x0001, 0x0792, 0x8a9d, 0x8fd5, 0x8b21, 0x8fd9, 0x0023, 0x00f1, 0x27b7, 0x9000, 0xa783, 0xfac7, 0x8b89, 0x8363, 0x1207, 0x07b7, 0x9300, 0xd703, 0x1927, 0x6795, 0x0742, 0x8341, 0x8793, 0x5aa7, 0x1a63, 0x00f7, 0x27b7, 0x9000, 0xa703, 0xfac7, 0x8793, 0xf847, 0x9b79, 0xd798, 0x0048, 0x1097, 0xf000, 0x80e7, 0xd120, 0x4509, 0x0097, 0xf000, 0x80e7, 0x6dc0, 0x450d, 0x0097, 0xf000, 0x80e7, 0x6aa0, 0x0097, 0xf000, 0x80e7, 0x5fe0, 0x1537, 0x0002, 0x0513, 0x8505, 0x1097, 0xf000, 0x80e7, 0x43e0, 0x07b7, 0x5fc1, 0x1737, 0x1802, 0x8793, 0x1007, 0xcf5c, 0x4705, 0x27b7, 0x9000, 0x82a3, 0xf8e7, 0x2737, 0x9000, 0xa011, 0x0001, 0x4483, 0xf857, 0x4785, 0xf493, 0x0ff4, 0x9ae3, 0xfef4, 0x02a3, 0xf807, 0x4501, 0x205d, 0x4505, 0x0097, 0xf000, 0x80e7, 0x4c60, 0x4665, 0x006c, 0x0513, 0x4000, 0x1097, 0xf000, 0x80e7, 0xcea0, 0x4605, 0x45e5, 0x0068, 0x3097, 0xf000, 0x80e7, 0x9780, 0x07b7, 0x9300, 0xd783, 0x1107, 0x0637, 0x9300, 0x1623, 0x00f1, 0x47b2, 0x4685, 0x8b9d, 0xf793, 0x0ff7, 0x1123, 0x00f1, 0x5783, 0x0021, 0x07c2, 0x83c1, 0x8463, 0x0297, 0x0001, 0x0001, 0x5783, 0x1106, 0x1623, 0x00f1, 0x47b2, 0x8b9d, 0xf793, 0x0ff7, 0x1123, 0x00f1, 0x5783, 0x0021, 0x07c2, 0x83c1, 0x91e3, 0xfed7, 0x4505, 0x2035, 0x2737, 0x9000, 0xb7b5, 0x4783, 0x0001, 0x8ba1, 0xe793, 0x0047, 0x0023, 0x00f1, 0xbdc9, 0x4503, 0x0c31, 0x893d, 0x0097, 0xf000, 0x80e7, 0x6060, 0xbdc1, 0x0000, 0x0000, 0x07b7, 0x9300, 0xd703, 0x1e47, 0x1141, 0xc606, 0xc426, 0x1323, 0x00e1, 0x0001, 0xd703, 0x1da7, 0x1223, 0x00e1, 0x0001, 0xd783, 0x1d67, 0x1123, 0x00f1, 0x0001, 0x4785, 0x0163, 0x06f5, 0x4485, 0x4505, 0x0001, 0x0097, 0xf000, 0x80e7, 0x6000, 0x0001, 0x5783, 0x0061, 0x9693, 0x0084, 0xf793, 0xef77, 0x9713, 0x0034, 0x8fd5, 0x8fd9, 0x07c2, 0x06b7, 0x9300, 0x83c1, 0x9223, 0x1ef6, 0x5703, 0x0041, 0x5783, 0x0021, 0x767d, 0x9b79, 0x167d, 0x8f45, 0x8ff1, 0x04b2, 0x8fc5, 0x0742, 0x8341, 0x07c2, 0x40b2, 0x9d23, 0x1ce6, 0x83c1, 0x9b23, 0x1cf6, 0x44a2, 0x0141, 0x8082, 0x4481, 0x450d, 0xb74d, 0x0000, 0x006d, 0x0154};
    // 同txt版本
    // dtof_uint16_t ram_code[] = {0x0113, 0xfb41, 0x0048, 0xc486, 0xc2a6, 0x1097, 0xf000, 0x80e7, 0xe0a0, 0x4509, 0x0097, 0xf000, 0x80e7, 0x7d40, 0x450d, 0x0097, 0xf000, 0x80e7, 0x7a20, 0x0097, 0xf000, 0x80e7, 0x6f60, 0x1537, 0x0002, 0x0513, 0x8505, 0x1097, 0xf000, 0x80e7, 0x5360, 0x07b7, 0x5fc1, 0x1737, 0x1802, 0x8793, 0x1007, 0xcf5c, 0x4705, 0x27b7, 0x9000, 0x82a3, 0xf8e7, 0x2737, 0x9000, 0xa011, 0x0001, 0x4483, 0xf857, 0x4785, 0xf493, 0x0ff4, 0x9ae3, 0xfef4, 0x02a3, 0xf807, 0x4501, 0x2049, 0x4505, 0x0097, 0xf000, 0x80e7, 0x5be0, 0x4665, 0x006c, 0x0513, 0x4000, 0x1097, 0xf000, 0x80e7, 0xde20, 0x4605, 0x45e5, 0x0068, 0x3097, 0xf000, 0x80e7, 0xa700, 0x07b7, 0x9300, 0xd783, 0x1107, 0x0637, 0x9300, 0x1623, 0x00f1, 0x47b2, 0x4685, 0x8b9d, 0xf793, 0x0ff7, 0x1123, 0x00f1, 0x5783, 0x0021, 0x07c2, 0x83c1, 0x8463, 0x0297, 0x0001, 0x0001, 0x5783, 0x1106, 0x1623, 0x00f1, 0x47b2, 0x8b9d, 0xf793, 0x0ff7, 0x1123, 0x00f1, 0x5783, 0x0021, 0x07c2, 0x83c1, 0x91e3, 0xfed7, 0x4505, 0x2021, 0x2737, 0x9000, 0xb7b5, 0x07b7, 0x9300, 0xd703, 0x1e47, 0x1141, 0xc606, 0xc426, 0x1323, 0x00e1, 0x0001, 0xd703, 0x1da7, 0x1223, 0x00e1, 0x0001, 0xd783, 0x1d67, 0x1123, 0x00f1, 0x0001, 0x4785, 0x0163, 0x06f5, 0x4485, 0x4505, 0x0001, 0x0097, 0xf000, 0x80e7, 0x71c0, 0x0001, 0x5783, 0x0061, 0x9693, 0x0084, 0xf793, 0xef77, 0x9713, 0x0034, 0x8fd5, 0x8fd9, 0x07c2, 0x06b7, 0x9300, 0x83c1, 0x9223, 0x1ef6, 0x5703, 0x0041, 0x5783, 0x0021, 0x767d, 0x9b79, 0x167d, 0x8f45, 0x8ff1, 0x04b2, 0x8fc5, 0x0742, 0x8341, 0x07c2, 0x40b2, 0x9d23, 0x1ce6, 0x83c1, 0x9b23, 0x1cf6, 0x44a2, 0x0141, 0x8082, 0x4481, 0x450d, 0xb74d, 0x0000, 0x006d, 0x00c6};
    // txt quit版本
    dtof_uint16_t ram_code[] = {0x0113, 0xfbc1, 0x669d, 0xde26, 0x0737, 0x9300, 0xc086, 0x8693, 0x7886, 0x1a23, 0x18d7, 0x24b7, 0x9000, 0xa783, 0xfb44, 0x8493, 0xf844, 0x17dd, 0x07c2, 0x83c1, 0x1223, 0x1cf7, 0x5783, 0x1907, 0x07c2, 0x83c1, 0x9bdd, 0x07c2, 0x83c1, 0x1823, 0x18f7, 0x0001, 0xd783, 0x0604, 0xd783, 0x0624, 0x0001, 0x4509, 0x82a3, 0x0004, 0x0097, 0xf000, 0x80e7, 0x7980, 0x450d, 0x0097, 0xf000, 0x80e7, 0x7660, 0x0097, 0xf000, 0x80e7, 0x6ba0, 0x1537, 0x0002, 0x0513, 0x8505, 0x1097, 0xf000, 0x80e7, 0x4fa0, 0x4785, 0x80a3, 0x00f4, 0x0737, 0x9300, 0x5783, 0x1947, 0x0737, 0x5fc1, 0x0713, 0x1007, 0x1637, 0x1802, 0xce58, 0x07c2, 0x671d, 0x83c1, 0x0693, 0x7887, 0x4485, 0x9363, 0x0ad7, 0x0001, 0x27b7, 0x9000, 0xc783, 0xf857, 0x2737, 0x9000, 0xf793, 0x0ff7, 0x8963, 0x0097, 0x0001, 0x4783, 0xf857, 0xf793, 0x0ff7, 0x9be3, 0xfe97, 0x27b7, 0x9000, 0x82a3, 0xf807, 0x4501, 0x2841, 0x4505, 0x0097, 0xf000, 0x80e7, 0x55c0, 0x4665, 0x004c, 0x0513, 0x4000, 0x1097, 0xf000, 0x80e7, 0xd800, 0x4605, 0x45e5, 0x0048, 0x3097, 0xf000, 0x80e7, 0xa0e0, 0x07b7, 0x9300, 0xd783, 0x1107, 0x0737, 0x9300, 0xa021, 0x0001, 0x5783, 0x1107, 0x1223, 0x00f1, 0x4792, 0x8b9d, 0xf793, 0x0ff7, 0x1123, 0x00f1, 0x5783, 0x0021, 0x07c2, 0x83c1, 0x91e3, 0xfe97, 0x4505, 0x2815, 0x07b7, 0x9300, 0xd703, 0x1947, 0x679d, 0x0742, 0x8341, 0x8793, 0x7887, 0x02e3, 0xf6f7, 0x6785, 0x4086, 0x0737, 0x9300, 0x8793, 0x2347, 0x1a23, 0x18f7, 0x54f2, 0x0113, 0x0441, 0x8082, 0x0000, 0x0000, 0x07b7, 0x9300, 0xd703, 0x1e47, 0x1141, 0xc606, 0xc426, 0x1323, 0x00e1, 0x0001, 0xd703, 0x1da7, 0x1223, 0x00e1, 0x0001, 0xd783, 0x1d67, 0x1123, 0x00f1, 0x0001, 0x4785, 0x0163, 0x06f5, 0x4485, 0x4505, 0x0001, 0x0097, 0xf000, 0x80e7, 0x6ac0, 0x0001, 0x5783, 0x0061, 0x9693, 0x0084, 0xf793, 0xef77, 0x9713, 0x0034, 0x8fd5, 0x8fd9, 0x07c2, 0x06b7, 0x9300, 0x83c1, 0x9223, 0x1ef6, 0x5703, 0x0041, 0x5783, 0x0021, 0x767d, 0x9b79, 0x167d, 0x8f45, 0x8ff1, 0x04b2, 0x8fc5, 0x0742, 0x8341, 0x07c2, 0x40b2, 0x9d23, 0x1ce6, 0x83c1, 0x9b23, 0x1cf6, 0x44a2, 0x0141, 0x8082, 0x4481, 0x450d, 0xb74d, 0x0000, 0x006d, 0x00fe};
    ret = dtof_ram_code_burn(ram_code, sizeof(ram_code)/sizeof(dtof_uint16_t), DTOF_SWB_TYPE_FROM_RAM_FIXADDR, 0);
    if(ret != DTOF_RET_SUCCESS){
        DTOF_LOG("file: %s, line: %d, ram code burn fail\n", __FILE__, __LINE__);
        return ret;
    }
    return ret;
}

// ram中每帧后输出dsp fifo, 然后bypass
DTOF_RET dtof_ram_output_dsp_fifo_debug(void)
{
    DTOF_RET ret;
    dtof_uint16_t ram_code[] = {0x07b7, 0x9300, 0xd783, 0x1107, 0x1151, 0x07c2, 0x83c1, 0xf713, 0x0077, 0x7713, 0x0ff7, 0x1023, 0x00e1, 0x5703, 0x0001, 0x1123, 0x00f1, 0x1793, 0x0107, 0xc406, 0xc226, 0x4585, 0x83c1, 0x0637, 0x9300, 0x4685, 0x8663, 0x02b7, 0x0001, 0x5783, 0x1106, 0x07c2, 0x83c1, 0xf713, 0x0077, 0x7713, 0x0ff7, 0x1023, 0x00e1, 0x5703, 0x0001, 0x1123, 0x00f1, 0x1793, 0x0107, 0x83c1, 0x9ee3, 0xfcd7, 0x2737, 0x9000, 0x2783, 0xfa47, 0x16b7, 0x1802, 0x0637, 0x5fc1, 0xced0, 0x4689, 0xc394, 0xc3d4, 0xc794, 0xc7d4, 0x06b7, 0x9000, 0x8693, 0x0a86, 0xa823, 0x0007, 0xa223, 0x0207, 0xd394, 0x4505, 0x0493, 0xf847, 0x0097, 0xf000, 0x80e7, 0x5a00, 0x40a2, 0x4785, 0xd0dc, 0x4492, 0x0131, 0x8082, 0x7139, 0x858a, 0x4665, 0x0513, 0x4000, 0xde06, 0x1097, 0xf000, 0x80e7, 0xdb40, 0x4605, 0x45e5, 0x850a, 0x3097, 0xf000, 0x80e7, 0xa420, 0x0073, 0x1050, 0x4505, 0x0097, 0xf000, 0x80e7, 0x5640, 0x27b7, 0x9000, 0xa783, 0xfa47, 0x50f2, 0x4709, 0xc7d8, 0x6121, 0x8082, 0x0000, 0x55aa, 0x0078};
    ret = dtof_ram_code_burn(ram_code, sizeof(ram_code)/sizeof(dtof_uint16_t), DTOF_SWB_TYPE_FROM_RAM_FIXADDR, 0);
    if(ret != DTOF_RET_SUCCESS){
        DTOF_LOG("file: %s, line: %d, ram code burn fail\n", __FILE__, __LINE__);
        return ret;
    }
    return ret;
}

// TODO: @倩雯 更新为最新的 ram code
DTOF_RET dtof_rom_checksum(void)
{
    DTOF_RET ret;
    dtof_uint16_t check_sum, reg0, temp;
    #include "src/ramcode/rom_checksum_data.ram"
    dtof_uint16_t ram_code_temp[sizeof(rom_checksum_code)/sizeof(dtof_uint16_t)];
    memcpy(ram_code_temp, rom_checksum_code, sizeof(rom_checksum_code));
    // 适用于cp测试，不需要等待两次中断，通过重启后从9000开始来做

    ret = dtof_set_mcu_status(DTOF_MCU_STATE_SLEEP_DIRECT);
    if(ret != DTOF_RET_SUCCESS){
        DTOF_LOG_ERR("ROM checksum, bypass error");
        return ret;
    }
    // 关jmpmode
    temp = 0x2098;

    ret |= dtof_reg_burst_write(device_id, DTOF_REG211, &temp, 1);
    if(ret != DTOF_RET_SUCCESS){
        DTOF_LOG_ERR("ROM checksum, close jmp mode error");
        return ret;
    }

    // 改启动地址为9000
    temp = 0x9000;
    ret |= dtof_reg_burst_write(device_id, DTOF_REG4, &temp, 1);
    if(ret != DTOF_RET_SUCCESS){
        DTOF_LOG_ERR("ROM checksum, change start addr error");
        return ret;
    }
    // 开始烧
    temp = 0x2000;
    ret |= dtof_reg_burst_write(device_id, DTOF_REG254, &temp, 1);

    ret = dtof_reg_burst_write(device_id, DTOF_READ_RAM_START_REG_ADDR, ram_code_temp, ROM_CHECKSUM_CODE_SIZE);
    if(ret != DTOF_RET_SUCCESS){
        DTOF_LOG_ERR("ROM checksum, burn fail");
        return ret;
    }

    // 重启
    temp = RESET_KEY_FROM_OUTTER;
    ret |= dtof_reg_burst_write(device_id, DTOF_REG5, &temp, 1);

    if(ret != DTOF_RET_SUCCESS){
        DTOF_LOG_ERR("ROM checksum, reset fail");
        return ret;
    }


    do {
        ret = dtof_reg_burst_read(device_id, DTOF_REG0, &reg0, 1);
        if(ret != DTOF_RET_SUCCESS) {
            DTOF_LOG_ERR("Register read failed");
            return ret;
        }
    } while(reg0 == DTOF_INVALID_REG_VALUE);

    ret = dtof_reg_burst_read(device_id, DTOF_REG110, &check_sum, 1);
    if(ret != DTOF_RET_SUCCESS) {
        DTOF_LOG_ERR("Checksum read failed");
        return ret;
    }
    #define ROM_CHECKSUM_VALID_VALUE 0x8888
    if(check_sum != ROM_CHECKSUM_VALID_VALUE) {
        DTOF_LOG_ERR("ROM checksum verification failed");
        return DTOF_RET_ERROR;
    }

    return ret;
}

#define DTOF_DEV_MODE
#ifdef DTOF_DEV_MODE
#undef DTOF_DEV_MODE

/**
 * @brief read register value
 * @param reg_addr register address
 * @param reg_data register value
 * @return DTOF_RET_SUCCESS or DTOF_FAIL
 * @note
 */
DTOF_RET dtof_read_reg_running(dtof_uint16_t reg_addr, dtof_uint16_t *reg_data)
{
    DTOF_RET ret;

    ret = dtof_io_interaction(device_id, DTOF_CMD_READ_REG, reg_addr);
    if(ret != DTOF_RET_SUCCESS){
        DTOF_LOG("file: %s, line: %d, send io cmd fail\n", __FILE__, __LINE__);
        return ret;
    }

    ret = dtof_reg_burst_read(device_id, DTOF_REG110, reg_data, 1);
    if(ret != DTOF_RET_SUCCESS){
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

    ret = dtof_io_interaction(device_id, DTOF_CMD_WRITE_REG_ADDR, reg_addr);
    if(ret != DTOF_RET_SUCCESS){
        DTOF_LOG("file: %s, line: %d, send io cmd fail\n", __FILE__, __LINE__);
        return ret;
    }

    ret = dtof_io_interaction(device_id, DTOF_CMD_WRITE_REG_LOW, reg_data & 0xFF);
    if(ret != DTOF_RET_SUCCESS){
        DTOF_LOG("file: %s, line: %d, send io cmd fail\n", __FILE__, __LINE__);
        return ret;
    }

    ret = dtof_io_interaction(device_id, DTOF_CMD_WRITE_REG_HIGH, (reg_data >> 8) & 0xFF);
    if(ret != DTOF_RET_SUCCESS){
        DTOF_LOG("file: %s, line: %d, send io cmd fail\n", __FILE__, __LINE__);
        return ret;
    }

    return ret;
}

#endif