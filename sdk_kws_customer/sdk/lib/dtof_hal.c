#pragma GCC diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"

#include <stdint.h>
#include "dtof_lib.h"
#include "dtof_reg.h"
#include "dtof_hal.h"
#include "inc/dtof_log.h"
#include "inc/dtof_base_type.h"
#include "inc/dtof_driver.h"

#define ENABLE_FSM   1
#define DISABLE_FSM  0

#define NEW_READ_RAM

DTOF_RET dtof_set_rngtime(dtof_uint8_t device_id, dtof_uint16_t rngtime)
{
    DTOF_RET ret;
    dtof_uint16_t temp;
    dtof_addressREG3_t *reg_p = (dtof_addressREG3_t *)&temp;

    ret = dtof_reg_burst_read(device_id, DTOF_REG3, &temp, 1);
    if(ret != DTOF_RET_SUCCESS){
        DTOF_LOG("file: %s, line: %d, reg read fail\n", __FILE__, __LINE__);
    }

    reg_p->rngTime = rngtime;

    ret = dtof_reg_burst_write(device_id, DTOF_REG3, &temp, 1);
    if(ret != DTOF_RET_SUCCESS){
        DTOF_LOG("file: %s, line: %d, reg read fail\n", __FILE__, __LINE__);
    }
    return ret;
}

DTOF_RET dtof_get_rngtime(dtof_uint8_t device_id, dtof_uint16_t* rngtime)
{
    DTOF_RET ret;
    dtof_uint16_t temp;
    dtof_addressREG3_t *reg_p = (dtof_addressREG3_t *)&temp;

    ret = dtof_reg_burst_read(device_id, DTOF_REG3, &temp, 1);
    if(ret != DTOF_RET_SUCCESS){
        DTOF_LOG("file: %s, line: %d, reg read fail\n", __FILE__, __LINE__);
    }

    *rngtime = reg_p->rngTime;
    return ret;
}


dtof_uint16_t dtof_fsm_state_get(dtof_uint8_t device_id)
{
// TODO:@liuzihao return ret
    DTOF_RET ret;
    dtof_uint16_t temp;
    dtof_addressREG136_t * reg_p = (dtof_addressREG136_t * )&temp;

    ret = dtof_reg_burst_read(device_id, DTOF_REG136, &temp, 1);
    if(ret != DTOF_RET_SUCCESS){
        DTOF_LOG("file: %s, line: %d, reg read fail\n", __FILE__, __LINE__);
    }

    return reg_p->fsm_state;
}


static DTOF_RET get_ram_address(dtof_uint8_t device_id, dtof_uint16_t* address)
{
    DTOF_RET ret = DTOF_RET_SUCCESS;
    dtof_uint16_t bnk_status;
    if (*address < 0x400)
    {
        ret = dtof_reg_burst_read(device_id, DTOF_REG137, &bnk_status, 1);
        if(ret != DTOF_RET_SUCCESS){
            DTOF_LOG("file: %s, line: %d, reg read fail\n", __FILE__, __LINE__);
            return ret;
        }

        if(bnk_status == RAMX0_OCCUPY){
            // if want to read RAMx_1, need set ram addr 12bit to 1, pull up ppRamFlg
            *address = *address | PULL_UP_12BIT;
        }
    }
    return ret;
}

DTOF_RET dtof_ram_read_lib(dtof_uint8_t device_id, dtof_uint16_t address, dtof_uint16_t * value_p, dtof_uint16_t len) {
    DTOF_RET ret;
    dtof_uint16_t index = 0;
    dtof_uint16_t temp;
    dtof_uint16_t burst_step = (len+7) / 8 ;
    dtof_uint16_t local_buffer[64];
    ret = dtof_reg_burst_read(device_id, DTOF_REG134, &temp, 1);
    if(ret != DTOF_RET_SUCCESS){
        DTOF_LOG("file: %s, line: %d, reg read fail\n", __FILE__, __LINE__);
        return ret;
    }

    ret = get_ram_address(device_id, &address);
    if(ret != DTOF_RET_SUCCESS){
        DTOF_LOG("file: %s, line: %d, get ram address fail\n", __FILE__, __LINE__);
        return ret;
    }

    if (((address & 0xfff) < 0x200) && ((temp & 0x800)==0))
    {

#if 1
       // main ram read
        for (dtof_uint16_t i = 0; i < 0x40; i++)
        {
            for (dtof_uint16_t j = 0; j < 8; j++)
            {
                // 两次for循环已经保证了最大只能是i*j了，不用再判断
                if (index >= len)
                {
                    return DTOF_RET_SUCCESS;
                }

                dtof_uint16_t ram_start_addr = address + (j * 0x40) + i;
                ret = dtof_reg_burst_write(device_id, DTOF_REG254, &ram_start_addr, 1);
                if(ret != DTOF_RET_SUCCESS){
                    DTOF_LOG("file: %s, line: %d, reg write fail\n", __FILE__, __LINE__);
                    return ret;
                }

                ret = dtof_reg_burst_read(device_id, DTOF_REG255, value_p + index, 1);
                if(ret != DTOF_RET_SUCCESS){
                    DTOF_LOG("file: %s, line: %d, reg read fail\n", __FILE__, __LINE__);
                    return ret;
                }

                index++;
            }
        }
#else
        for (dtof_uint16_t j = 0; j < 8; j++)
        {
            // 获取连续的数据
            dtof_uint16_t ram_start_addr = (j * 0x40);
            ret = dtof_reg_burst_write(device_id, DTOF_REG254, &ram_start_addr, 1);
            if(ret != DTOF_RET_SUCCESS){
                DTOF_LOG("file: %s, line: %d, reg write fail\n", __FILE__, __LINE__);
                return ret;
            }

            ret = dtof_reg_burst_read(device_id, DTOF_REG255, local_buffer, burst_step);
            if(ret != DTOF_RET_SUCCESS){
                DTOF_LOG("file: %s, line: %d, reg read fail\n", __FILE__, __LINE__);
                return ret;
            }
            for (dtof_uint16_t i = 0; i < burst_step; i++)
            {
                int index = i * 8 + j;
                if (index >= len)
                {
                    continue;
                }
                value_p[index] = local_buffer[i];
            }
        }
#endif



        // (*((volatile dtof_uint16_t *)(DEFAULT_LLDSP_BASE+DRIVER_RAM_STARTA_ADD*DRIVER_REG_MULTIPLE))) = address + *0x40;
    }else{
        ret = dtof_reg_burst_write(device_id, DTOF_REG254, &address, 1);
        if(ret != DTOF_RET_SUCCESS){
            DTOF_LOG("file: %s, line: %d, reg write fail\n", __FILE__, __LINE__);
            return ret;
        }

        // read the value from 0xff
        for(; index < len ; index++) {
            ret = dtof_reg_burst_read(device_id, DTOF_REG255, value_p + index, 1);
            if(ret != DTOF_RET_SUCCESS){
                DTOF_LOG("file: %s, line: %d, reg burst read fail\n", __FILE__, __LINE__);
            }
        }
    }
    return ret;
}

DTOF_RET dtof_histgram_io_read_lib(dtof_uint8_t device_id, dtof_uint16_t address, dtof_uint16_t * value_p, dtof_uint16_t len) {
    DTOF_RET ret;
    ret = dtof_ram_read_lib(device_id, address, value_p, len);
    if(ret != DTOF_RET_SUCCESS){
        DTOF_LOG("file: %s, line: %d, histgram read fail\n", __FILE__, __LINE__);
        return ret;
    }
    return ret;
}

DTOF_RET dtof_fsm_change(dtof_uint8_t device_id, dtof_uint32_t runmode)
{
    DTOF_RET ret;
    dtof_uint16_t temp;
    dtof_addressREG3_t * reg_p = (dtof_addressREG3_t*)&temp;

    ret = dtof_reg_burst_read(device_id, DTOF_REG3, &temp, 1);
    if(ret != DTOF_RET_SUCCESS){
        DTOF_LOG("file: %s, line: %d, reg read fail\n", __FILE__, __LINE__);
        return ret;
    }

    if(runmode == RUN_MODE_HOLD){
        // 在idle和fdly才可以关状态机
        while(dtof_fsm_state_get(device_id) > FSM_STATE_FDLY){
            #if defined(__CC_ARM) || defined(__CLANG_ARM) || defined (__clang__)
            __asm__("nop");
            #elif defined (__ICCARM__) || defined(__ICCRX__)
                __asm("nop");
            #elif defined (__GNUC__)
                __asm__ __volatile__("nop");
            #else
                #error "unsupported compiler"
            #endif
        }
        reg_p->cfgDoneR = DISABLE_FSM;
        reg_p->runMod = runmode;
    }
    else{
        reg_p->cfgDoneR = ENABLE_FSM;
        reg_p->runMod = runmode;
    }

    ret = dtof_reg_burst_write(device_id, DTOF_REG3, &temp, 1);
    if(ret != DTOF_RET_SUCCESS){
        DTOF_LOG("file: %s, line: %d, reg write fail\n", __FILE__, __LINE__);
        return ret;
    }

    return ret;
}

DTOF_RET hal_dtof_prepare_one_frame(dtof_uint8_t device_id)
{
    DTOF_RET ret;

    ret = dtof_fsm_change(device_id, RUN_MODE_RUNONCE);
    if(ret != DTOF_RET_SUCCESS){
        DTOF_LOG("file: %s, line: %d, change fsm status fail\n", __FILE__, __LINE__);
        return ret;
    }

    while(dtof_fsm_state_get(device_id) > FSM_STATE_FDLY){
        #if defined(__CC_ARM) || defined(__CLANG_ARM) || defined (__clang__)
            __asm__("nop");
        #elif defined (__ICCARM__) || defined(__ICCRX__)
            __asm("nop");
        #elif defined (__GNUC__)
            __asm__ __volatile__("nop");
        #else
            #error "unsupported compiler"
        #endif
    }

    return ret;
}

DTOF_RET dtof_calibration_get_frame_data(dtof_uint8_t device_id, uint16_t offset, uint16_t *out_buf, uint16_t len)
{
    DTOF_RET ret;
    if (!out_buf)
    {
        DTOF_LOG("param error\r\n");
        return DTOF_RET_ERROR;
    }

    ret = hal_dtof_prepare_one_frame(device_id);
    DTOF_CHECK_RET(ret, "prepare one frame fail\n");

    ret = dtof_histgram_io_read_lib(device_id, offset, out_buf, len);
    DTOF_CHECK_RET(ret, "read ram fail\n");

    return ret;
}

/**
 * @brief read register value
 * @param reg_addr register address
 * @param reg_data register value
 * @return DTOF_RET_SUCCESS or DTOF_FAIL
 * @note
 */
DTOF_RET dtof_read_reg_running_lib(dtof_uint8_t device_id, dtof_uint16_t reg_addr, dtof_uint16_t *reg_data)
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
DTOF_RET dtof_write_reg_running_lib(dtof_uint8_t device_id, dtof_uint16_t reg_addr, dtof_uint16_t reg_data)
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

DTOF_RET hal_dtof_maxfls_get(dtof_uint8_t device_id, dtof_uint16_t *maxfls)
{
    DTOF_RET ret;

    if(!maxfls){
        DTOF_LOG("file: %s, line: %d, pointer is null\n", __FILE__, __LINE__);
        return DTOF_RET_ERROR;
    }

    ret = dtof_reg_burst_read(device_id, DTOF_FLASHCOUNT_ADDR, maxfls, 1);
    if(ret != DTOF_RET_SUCCESS){
        DTOF_LOG("file: %s, line: %d, reg read fail\n", __FILE__, __LINE__);
        return ret;
    }
    return ret;
}

DTOF_RET hal_dtof_maxfls_config(dtof_uint8_t device_id, dtof_uint16_t maxfls)
{
    DTOF_RET ret;
    ret = dtof_reg_burst_write(device_id, DTOF_FLASHCOUNT_ADDR, &maxfls, 1);
    if(ret != DTOF_RET_SUCCESS){
        DTOF_LOG("file: %s, line: %d, reg write fail\n", __FILE__, __LINE__);
        return ret;
    }
    return ret;
}
