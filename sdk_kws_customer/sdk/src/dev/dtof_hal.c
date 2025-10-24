/*
 * dtof_hal.c
 *
 *  Created on: 2024/10/24
 *      Author: liuzihao
 */

#include "sdk/inc/dtof_base_type.h"
#include "inc/dtof_driver.h"
#include "sdk/inc/dev/dtof_reg.h"
#include "sdk/inc/dtof_common.h"
#include "sdk/inc/dtof_log.h"
#include "inc/dev/dtof_hal.h"

DTOF_RET hal_cg_config(dtof_bool_t status){
    DTOF_RET ret;
    dtof_uint16_t cg;
    dtof_addressREG3_t * reg3_p = (dtof_addressREG3_t*)&cg;

    ret = dtof_reg_burst_read(device_id, DTOF_REG3, &cg, 1);
    if(ret != DTOF_RET_SUCCESS){
        DTOF_LOG("file: %s, line: %d, reg read fail\n", __FILE__, __LINE__);
        return ret;
    }

    reg3_p->rmvNeXt = status;
    reg3_p->bstNeXt = status;

    ret = dtof_reg_burst_write(device_id, DTOF_REG3, &cg, 1);
    if(ret != DTOF_RET_SUCCESS){
        DTOF_LOG("file: %s, line: %d, reg write fail\n", __FILE__, __LINE__);
        return ret;
    }

    return ret;
}

DTOF_RET hal_dtof_maxfls_config(dtof_uint16_t maxfls)
{
    DTOF_RET ret;
    ret = dtof_reg_burst_write(device_id, DTOF_FLASHCOUNT_ADDR, &maxfls, 1);
    if(ret != DTOF_RET_SUCCESS){
        DTOF_LOG("file: %s, line: %d, reg write fail\n", __FILE__, __LINE__);
        return ret;
    }
    return ret;
}

DTOF_RET hal_dtof_maxfls_get(dtof_uint16_t *maxfls)
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

DTOF_RET hal_ref_spad_mask_config(dtof_uint16_t ref_spad_mask)
{
    DTOF_RET ret;
    dtof_uint16_t spad_enable = 0;

    ret = dtof_reg_burst_write(device_id, DTOF_REG208, &ref_spad_mask, 1);
    if(ret != DTOF_RET_SUCCESS){
        DTOF_LOG("file: %s, line: %d, reg write fail\n", __FILE__, __LINE__);
        return ret;
    }

    ret = dtof_reg_burst_read(device_id, DTOF_REG209, &spad_enable, 1);
    if(ret != DTOF_RET_SUCCESS){
        DTOF_LOG("file: %s, line: %d, reg write fail\n", __FILE__, __LINE__);
        return ret;
    }

    DTOF_BIT_FLIP(spad_enable, 0);

    ret = dtof_reg_burst_write(device_id, DTOF_REG209, &spad_enable, 1);
    if(ret != DTOF_RET_SUCCESS){
        DTOF_LOG("file: %s, line: %d, reg write fail\n", __FILE__, __LINE__);
        return ret;
    }

    return ret;
}

DTOF_RET hal_next_ac_data_config(dtof_uint16_t *next_ac_data_p, dtof_uint16_t length)
{
    DTOF_RET ret;

    if(!next_ac_data_p){
        DTOF_LOG("file: %s, line: %d, pointer is null\n", __FILE__, __LINE__);
        return DTOF_RET_ERROR;
    }

    ret = dtof_reg_burst_write(device_id, DTOF_REG172, next_ac_data_p, length);
    if(ret != DTOF_RET_SUCCESS){
        DTOF_LOG("file: %s, line: %d, reg write fail\n", __FILE__, __LINE__);
        return ret;
    }
    return ret;
}

DTOF_RET hal_next_dc_data_config(dtof_uint16_t next_dc_data)
{
    DTOF_RET ret;
    ret = dtof_reg_burst_write(device_id, DTOF_REG188, &next_dc_data, 1);
    if(ret != DTOF_RET_SUCCESS){
        DTOF_LOG("file: %s, line: %d, reg write fail\n", __FILE__, __LINE__);
        return ret;
    }
    return ret;
}

#define OTP_ENABLE 0x0020
#define OTP_DISABLE 0x0000
#define MVPP_AND_PPROG_ENABLE 0x0031     // after otp enable
#define PWE_ENABLE 0x0033                // after otp, mvpp, pprog enable and write PA PDIN
#define PWE_DISABLE 0x0031               // first disable pwe after write otp
#define OTP_MVPP_AND_PPROG_DIABLE 0x0000 // disable otp, mvpp, pprog after disable pwe
static void otp_read_enable(dtof_bool_t status)
{
    dtof_uint16_t otp_temp;
    if (status == DTOF_FALSE)
    {
        otp_temp = OTP_DISABLE;
        dtof_reg_burst_write(device_id, DTOF_REG251, &otp_temp, 1);
    }
    else
    {
        // enable
        otp_temp = OTP_ENABLE;
        dtof_reg_burst_write(device_id, DTOF_REG251, &otp_temp, 1);
    }
    return;
}

static void otp_write_enable(dtof_bool_t status)
{
    dtof_uint16_t otp_temp;
    if (status == DTOF_FALSE)
    {
        otp_temp = OTP_MVPP_AND_PPROG_DIABLE;
        dtof_reg_burst_write(device_id, DTOF_REG251, &otp_temp, 1);
    }
    else
    {
        // enable iic speed, do not need delay
        otp_temp = OTP_ENABLE;
        dtof_reg_burst_write(device_id, DTOF_REG251, &otp_temp, 1);
        // enable HVPP
        otp_temp = MVPP_AND_PPROG_ENABLE;
        dtof_reg_burst_write(device_id, DTOF_REG251, &otp_temp, 1);
    }

    return;
}
// use dtof_uint16_t to get buf
DTOF_RET dtof_read_otp(dtof_uint8_t offset, dtof_uint8_t *buf, dtof_uint16_t len){
    DTOF_RET ret;
    dtof_uint16_t temp;
    temp = INNER_OTP_START_ADDR + offset;
    otp_read_enable(DTOF_TRUE);
    ret = dtof_reg_burst_write(device_id, DTOF_REG254, &temp, 1);
    if(ret != DTOF_RET_SUCCESS)
    {
        return ret;
    }
    for (dtof_uint16_t i = 0; i < len; i++)
    {
        ret = dtof_reg_burst_read(device_id, DTOF_REG255, &temp, 1);
        if(ret != DTOF_RET_SUCCESS)
        {
            return ret;
        }
        *(buf + i) =  temp&0xff;
    }
    temp = 0x0000;
    ret = dtof_reg_burst_write(device_id, DTOF_REG254, &temp, 1);
    otp_read_enable(DTOF_FALSE);
		return ret;
}

DTOF_RET dtof_write_otp(dtof_uint8_t offset, dtof_uint16_t len, dtof_uint8_t *out_buf){
    DTOF_RET ret;
    dtof_uint16_t temp;
    otp_write_enable(DTOF_TRUE);
    for (dtof_uint16_t index = 0; index < len; index++)
        {
        temp = (offset + index) + (*(out_buf + index) << 8);
        ret = dtof_reg_burst_write(device_id, DTOF_REG252, &temp, 1);
        if(ret != DTOF_RET_SUCCESS)
        {
            return ret;
        }
        {
            temp = PWE_ENABLE;
            ret = dtof_reg_burst_write(device_id, DTOF_REG251, &temp, 1);
            temp = PWE_DISABLE;
            ret = dtof_reg_burst_write(device_id, DTOF_REG251, &temp, 1);
        }
    }
    otp_write_enable(DTOF_FALSE);
    return DTOF_RET_SUCCESS;
}

DTOF_RET hal_dtof_burst_read_ram(dtof_uint16_t start_addr, dtof_uint16_t len, dtof_uint16_t *buf)
{
    DTOF_RET ret = DTOF_RET_SUCCESS;

    ret |= dtof_reg_burst_write(device_id, DTOF_REG254, &start_addr, 1);
    ret |= dtof_reg_burst_read(device_id, DTOF_REG255, buf, len);

    return ret;
}

DTOF_RET hal_dtof_burst_write_ram(dtof_uint16_t start_addr, dtof_uint16_t len, dtof_uint16_t *buf)
{
    DTOF_RET ret = DTOF_RET_SUCCESS;

    ret |= dtof_reg_burst_write(device_id, DTOF_REG254, &start_addr, 1);
    ret |= dtof_reg_burst_write(device_id, DTOF_REG255, buf, len);

    return ret;
}

static DTOF_RET get_ram_address(dtof_uint16_t* address)
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

DTOF_RET dtof_ram_read(dtof_uint16_t address, dtof_uint16_t * value_p, dtof_uint16_t len) {
    DTOF_RET ret;
    dtof_uint16_t index = 0;
    dtof_uint16_t temp;

    ret = dtof_reg_burst_read(device_id, DTOF_REG134, &temp, 1);
    if(ret != DTOF_RET_SUCCESS){
        DTOF_LOG("file: %s, line: %d, reg read fail\n", __FILE__, __LINE__);
        return ret;
    }

    ret = get_ram_address(&address);
    if(ret != DTOF_RET_SUCCESS){
        DTOF_LOG("file: %s, line: %d, get ram address fail\n", __FILE__, __LINE__);
        return ret;
    }

    if (((address & 0xfff) < 0x200) && ((temp & 0x800)==0))
    {
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

DTOF_RET dtof_histgram_io_read(dtof_uint16_t address, dtof_uint16_t * value_p, dtof_uint16_t len) {
    DTOF_RET ret;
    ret = dtof_ram_read(address, value_p, len);
    if(ret != DTOF_RET_SUCCESS){
        DTOF_LOG("file: %s, line: %d, histgram read fail\n", __FILE__, __LINE__);
        return ret;
    }
    return ret;
}

DTOF_RET dtof_dsp_fifo_read(dtof_uint16_t addr_offset, dtof_uint16_t * value_p, dtof_uint16_t len){
    DTOF_RET ret;
    ret = dtof_ram_read(DTOF_FIFO_START_ADDR + addr_offset, value_p, len);
    if(ret != DTOF_RET_SUCCESS){
        DTOF_LOG("file: %s, line: %d, dsp fifo read fail\n", __FILE__, __LINE__);
        return ret;
    }
    return ret;
}

dtof_uint16_t dtof_fsm_state_get(void)
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

DTOF_RET dtof_fsm_change(dtof_uint32_t runmode)
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
        while(dtof_fsm_state_get() > FSM_STATE_FDLY){
            __asm__("nop");
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

DTOF_RET hal_dtof_prepare_one_frame(void)
{
    DTOF_RET ret;

    // TODO:@liuzihao 2 type: wait intr or wait fsm
    ret = dtof_fsm_change(RUN_MODE_RUNONCE);
    if(ret != DTOF_RET_SUCCESS){
        DTOF_LOG("file: %s, line: %d, change fsm status fail\n", __FILE__, __LINE__);
        return ret;
    }

    while(dtof_fsm_state_get() > FSM_STATE_FDLY){
        __asm__("nop");
    }

    return ret;
}

DTOF_RET hal_dtof_set_sleep(dtof_uint16_t status)
{
    DTOF_RET ret;
    dtof_uint16_t temp;
    dtof_addressREG3_t * reg_p = (dtof_addressREG3_t*)&temp;

    ret = dtof_reg_burst_read(device_id, DTOF_REG3, &temp, 1);
    if(ret != DTOF_RET_SUCCESS){
        DTOF_LOG("file: %s, line: %d, reg read fail\n", __FILE__, __LINE__);
        return ret;
    }

    reg_p->sleep = status;

    ret = dtof_reg_burst_write(device_id, DTOF_REG3, &temp, 1);
    if(ret != DTOF_RET_SUCCESS){
        DTOF_LOG("file: %s, line: %d, reg write fail\n", __FILE__, __LINE__);
        return ret;
    }

    return ret;
}

DTOF_RET hal_dtof_reset(dtof_uint16_t reset_type)
{
    DTOF_RET ret;
    dtof_uint16_t temp;
    dtof_addressREG117_t * reg_p = (dtof_addressREG117_t*)&temp;

    ret = dtof_reg_burst_read(device_id, DTOF_REG117, &temp, 1);
    if(ret != DTOF_RET_SUCCESS){
        DTOF_LOG("file: %s, line: %d, reg read fail\n", __FILE__, __LINE__);
        return ret;
    }

    if(reset_type == RESET_TYPE_SOFT){
        reg_p->softRst = 1;
        ret = dtof_reg_burst_write(device_id, DTOF_REG117, &temp, 1);
        if(ret != DTOF_RET_SUCCESS){
            DTOF_LOG("file: %s, line: %d, reg write fail\n", __FILE__, __LINE__);
            return ret;
        }

        reg_p->softRst = 0;
        ret = dtof_reg_burst_write(device_id, DTOF_REG117, &temp, 1);
        if(ret != DTOF_RET_SUCCESS){
            DTOF_LOG("file: %s, line: %d, reg write fail\n", __FILE__, __LINE__);
            return ret;
        }
    }
    // TODO:@liuzihao global reset tbd
    return ret;
}
