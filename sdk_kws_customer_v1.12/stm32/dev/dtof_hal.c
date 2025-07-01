#include "inc/dtof_base_type.h"
#include "inc/dtof_driver.h"
#include "inc/dtof_log.h"
#include "dtof_reg.h"
#include "dtof_hal.h"

static DTOF_RET get_ram_address(dtof_uint16_t* address)
{
    DTOF_RET ret = DTOF_RET_SUCCESS;
    dtof_uint16_t bnk_status;
    if (*address < 0x400)
    {
        ret = dtof_reg_burst_read(0, DTOF_REG137, &bnk_status, 1);
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

    ret = dtof_reg_burst_read(0, DTOF_REG134, &temp, 1);
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
                ret = dtof_reg_burst_write(0, DTOF_REG254, &ram_start_addr, 1);
                if(ret != DTOF_RET_SUCCESS){
                    DTOF_LOG("file: %s, line: %d, reg write fail\n", __FILE__, __LINE__);
                    return ret;
                }

                ret = dtof_reg_burst_read(0, DTOF_REG255, value_p + index, 1);
                if(ret != DTOF_RET_SUCCESS){
                    DTOF_LOG("file: %s, line: %d, reg read fail\n", __FILE__, __LINE__);
                    return ret;
                }

                index++;
            }
        }
        // (*((volatile dtof_uint16_t *)(DEFAULT_LLDSP_BASE+DRIVER_RAM_STARTA_ADD*DRIVER_REG_MULTIPLE))) = address + *0x40;
    }else{
        ret = dtof_reg_burst_write(0, DTOF_REG254, &address, 1);
        if(ret != DTOF_RET_SUCCESS){
            DTOF_LOG("file: %s, line: %d, reg write fail\n", __FILE__, __LINE__);
            return ret;
        }

        // read the value from 0xff
        for(; index < len ; index++) {
            ret = dtof_reg_burst_read(0, DTOF_REG255, value_p + index, 1);
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
#define DTOF_FIFO_START_ADDR   0x0400
    DTOF_RET ret;
    ret = dtof_ram_read(DTOF_FIFO_START_ADDR + addr_offset, value_p, len);
    if(ret != DTOF_RET_SUCCESS){
        DTOF_LOG("file: %s, line: %d, dsp fifo read fail\n", __FILE__, __LINE__);
        return ret;
    }
    return ret;
}

DTOF_RET dtof_read_reg_running(dtof_uint16_t reg_addr, dtof_uint16_t *reg_data)
{
    DTOF_RET ret;

    ret = dtof_io_interaction(0, DTOF_CMD_READ_REG, reg_addr);
    if(ret != DTOF_RET_SUCCESS){
        DTOF_LOG("file: %s, line: %d, send io cmd fail\n", __FILE__, __LINE__);
        return ret;
    }

    ret = dtof_reg_burst_read(0, DTOF_REG110, reg_data, 1);
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

    ret = dtof_io_interaction(0, DTOF_CMD_WRITE_REG_ADDR, reg_addr);
    if(ret != DTOF_RET_SUCCESS){
        DTOF_LOG("file: %s, line: %d, send io cmd fail\n", __FILE__, __LINE__);
        return ret;
    }

    ret = dtof_io_interaction(0, DTOF_CMD_WRITE_REG_LOW, reg_data & 0xFF);
    if(ret != DTOF_RET_SUCCESS){
        DTOF_LOG("file: %s, line: %d, send io cmd fail\n", __FILE__, __LINE__);
        return ret;
    }

    ret = dtof_io_interaction(0, DTOF_CMD_WRITE_REG_HIGH, (reg_data >> 8) & 0xFF);
    if(ret != DTOF_RET_SUCCESS){
        DTOF_LOG("file: %s, line: %d, send io cmd fail\n", __FILE__, __LINE__);
        return ret;
    }

    return ret;
}
