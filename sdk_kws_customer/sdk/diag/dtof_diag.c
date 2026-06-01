#include "inc/dtof_base_type.h"
#include "inc/dtof_driver.h"
#include "inc/dtof_log.h"
#include "inc/dtof_api.h"
#include "inc/dtof_global_config.h"
#include "inc/dtof_endian.h"
#include "inc/dtof_common.h"

static void convert_endian(uint16_t *data, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++) {
        data[i] = DTOF_SWAP16(data[i]);
    }
}

DTOF_RET dtof_reg_read_and_write_test(void)
{
    dtof_uint16_t temp;

    DTOF_CHECK_RET(dtof_reg_burst_read(DTOF_CHIP_ID_REG_ADDR, &temp, 1), "reg read fail\n");
    if(temp != DTOF_INVALID_REG_VALUE)
    {
        DTOF_LOG_ERR("test reg read fail, reg 0x00 = 0x%04x, expected value = 0x%04x\n", temp, DTOF_INVALID_REG_VALUE);
        return DTOF_RET_ERROR;
    }
    DTOF_LOG("reg read test pass, reg 0x00 = 0x%04x\n", temp);

    temp = DTOF_SET_MCU_SLEEP_VALUE;

    DTOF_CHECK_RET(dtof_reg_burst_write(DTOF_IO_CTRL_REG_ADDR, &temp, 1), "reg write fail\n");

    DTOF_CHECK_RET(dtof_reg_burst_read(DTOF_IO_CTRL_REG_ADDR, &temp, 1), "reg read fail\n");
    if(temp != DTOF_SET_MCU_SLEEP_VALUE)
    {
        DTOF_LOG_ERR("test reg write fail, reg 0x05 = 0x%04x, expected value = 0x%04x\n", temp, DTOF_SET_MCU_SLEEP_VALUE);
        return DTOF_RET_ERROR;
    }
    DTOF_LOG("reg write test pass, reg 0x05 = 0x%04x\n", temp);

    DTOF_CHECK_RET(dtof_reg_burst_read(DTOF_CHIP_ID_REG_ADDR, &temp, 1), "reg read fail\n");

    DTOF_LOG("chip id = 0x%04x\n", temp);

    return DTOF_RET_SUCCESS;
}

DTOF_RET dtof_reg_burst_write_burn_test(void)
{
#define TEST_RAM_CODE_LEN 2212
    const dtof_uint16_t *ram_code_ptr;
    dtof_uint16_t ram_code_len;
    dtof_uint16_t dtof_ft_data_start = DTOF_RAM_START_ADDR;
    dtof_uint16_t ram_data[TEST_RAM_CODE_LEN];

    dtof_get_ram_code_table(2, &ram_code_ptr, &ram_code_len);

    DTOF_CHECK_RET(dtof_reg_burst_write(DTOF_WRITE_RAM_START_REG_ADDR, &dtof_ft_data_start, 1), "write ram start addr failed\n");
    DTOF_CHECK_RET(dtof_reg_burst_write_burn(DTOF_READ_RAM_START_REG_ADDR, ram_code_ptr, ram_code_len), "write ft data failed\n");

    DTOF_CHECK_RET(dtof_reg_burst_write(DTOF_WRITE_RAM_START_REG_ADDR, &dtof_ft_data_start, 1), "write ram start addr failed\n");
    DTOF_CHECK_RET(dtof_reg_burst_read(DTOF_READ_RAM_START_REG_ADDR, ram_data, ram_code_len), "read ft data failed\n");

    convert_endian(ram_data, ram_code_len);

    for(int i = 0; i < ram_code_len; i++)
    {
        if(ram_data[i] != ram_code_ptr[i])
        {
            DTOF_LOG_ERR("test reg burst write burn fail, ram data[%d] = 0x%04x, expected value = 0x%04x\n", i, ram_data[i], ram_code_ptr[i]);
            return DTOF_RET_ERROR;
        }
    }

    DTOF_LOG("reg burst write burn test pass\n");

    return DTOF_RET_SUCCESS;
}

DTOF_RET dtof_read_and_write_ft_data_test(void)
{
    dtof_bool_t is_legal_data;
    dtof_ft_cali_param_t dtof_ft_test_read_data[RUNNING_MODE_NUM];
    dtof_ft_cali_param_t dtof_ft_test_write_data[RUNNING_MODE_NUM] = {
    {
        .ft_calibration_type = 1,
        .dtof_ft_data = {
            .cg_data = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34},
            .distance_k = 100,
            .distance_b = 10,
            .ref_spad = 20,
            .bin_offset = 5,
        },
    },

    {
        .ft_calibration_type = 2,
        .dtof_ft_data = {
            .cg_data = {2,4,6,8,10,12,14,16,18,20,22,24,26,28,30,32,34,36,38,40,42,44,46,48,50,52,54,56,58,60,62,64,66,68},
            .distance_k = 200,
            .distance_b = 20,
            .ref_spad = 30,
            .bin_offset = 6,
        },
    },

    {
        .ft_calibration_type = 3,
        .dtof_ft_data = {
            .cg_data = {3,6,9,12,15,18,21,24,27,30,33,36,39,42,45,48,51,54,57,60,63,66,69,72,75,78,81,84,87,90,93,96,99,102},
            .distance_k = 300,
            .distance_b = 30,
            .ref_spad = 40,
            .bin_offset = 7,
        },
    }};

    for(int i = 0; i < RUNNING_MODE_NUM; i++)
    {
        is_legal_data = DTOF_FALSE;

        DTOF_CHECK_RET(dtof_set_ft_data_to_flash_multi_mode((dtof_uint16_t *)(&dtof_ft_test_write_data[i]), sizeof(dtof_ft_cali_param_t) / sizeof(dtof_uint16_t), i), "set ft data to flash failed\n");
        DTOF_CHECK_RET(dtof_get_ft_data_from_flash_multi_mode((dtof_uint16_t *)(&dtof_ft_test_read_data[i]), sizeof(dtof_ft_cali_param_t) / sizeof(dtof_uint16_t), i, &is_legal_data), "get ft data from flash failed\n");

        if(is_legal_data == DTOF_FALSE)
        {
            DTOF_LOG_ERR("ft data is not legal for running mode %d\n", i);
            return DTOF_RET_ERROR;
        }

        if(dtof_memcmp(&dtof_ft_test_write_data[i], &dtof_ft_test_read_data[i], sizeof(dtof_ft_cali_param_t)) != 0)
        {
            DTOF_LOG_ERR("test read and write ft data fail for running mode %d\n", i);
            return DTOF_RET_ERROR;
        }

        DTOF_LOG("read and write ft data test pass for running mode %d\n", i);
    }

    return DTOF_RET_SUCCESS;
}


DTOF_RET dtof_diag_test(void)
{
    DTOF_RET ret = DTOF_RET_SUCCESS;
    ret |= dtof_reg_read_and_write_test();
    ret |= dtof_reg_burst_write_burn_test();
    ret |= dtof_read_and_write_ft_data_test();

    if(ret == DTOF_RET_SUCCESS)
    {
        DTOF_LOG("diag test pass\n");
    }
    return ret;
}
