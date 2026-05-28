/*
 * dtof_driver.c
 *
 *  Created on: 2024/8/5
 *      Author: liuzihao
 */
#include "inc/dtof_common.h"
#include "inc/dtof_log.h"
#include "inc/dtof_driver.h"

/**
 * @brief burst read register
 * @param[in] reg_addr register address
 * @param[out] reg_data_p register data pointer
 * @param[in] len data length
 * @return DTOF_RET_SUCCESS success, DTOF_RET_FAILED fail
 */
DTOF_RET DTOF_WEAK dtof_reg_burst_read(dtof_uint8_t reg_addr, dtof_uint16_t *reg_data_p, dtof_uint16_t len)
{
    DTOF_LOG("reg burst read use weak func!\n");
    return DTOF_RET_SUCCESS;
}

/**
 * @brief burst write to register
 * @param[in] reg_addr register address
 * @param[in] reg_data_p pointer to the data to be written
 * @param[in] len length of the data
 * @return DTOF_RET_SUCCESS on success, DTOF_RET_FAILED on failure
 */
DTOF_RET DTOF_WEAK dtof_reg_burst_write(dtof_uint8_t reg_addr, dtof_uint16_t *reg_data_p, dtof_uint16_t len)
{
    DTOF_LOG("reg burst write use weak func!\n");
    return DTOF_RET_SUCCESS;
}

/**
 * @brief burst write to upgrade
 * @param[in] reg_addr register address
 * @param[in] reg_data_p pointer to the data to be written
 * @param[in] len length of the data
 * @return DTOF_RET_SUCCESS on success, DTOF_RET_FAILED on failure
 */
DTOF_RET DTOF_WEAK dtof_reg_burst_write_burn(dtof_uint8_t reg_addr, const dtof_uint16_t *reg_data_p, dtof_uint16_t len)
{
    DTOF_LOG("reg burst write burn use weak func!\n");

    return DTOF_RET_SUCCESS;
}

/**
 * @brief IO interaction
 * @param[in] cmd command value
 * @param[in] value argument value
 * @return DTOF_RET_SUCCESS success, DTOF_RET_FAILED fail
 * Send a command to the sensor and wait for the response.
 */
DTOF_RET dtof_io_interaction(dtof_uint16_t cmd, dtof_uint16_t value)
{
    DTOF_RET ret;
    dtof_uint16_t reg_data = PACK_IO_INTERACTION_DATA(cmd, value);
    ret = dtof_reg_burst_write(DTOF_IO_CTRL_REG_ADDR, &reg_data, 1);
    return ret;
}

/**
 * @brief Sets the interrupt flag to the specified value.
 * @param[in] interrupt flag
 */
void DTOF_WEAK dtof_set_interrupt_flag(dtof_bool_t flag)
{
    return;
}

/**
 * @brief Get the interrupt flag value.
 * @return DTOF_TRUE if there is an interrupt, DTOF_FALSE otherwise.
 */
dtof_bool_t DTOF_WEAK dtof_get_interrupt_flag(void)
{
    return 0;
}

/**
 * @brief Sleep for a specified number of milliseconds.
 * @param[in] time Number of milliseconds to sleep.
 */
void DTOF_WEAK dtof_sleep_ms(dtof_uint32_t time)
{
    return;
}

DTOF_RET DTOF_WEAK dtof_get_ft_data_from_flash_multi_mode(dtof_uint16_t *ft_data, dtof_uint16_t len, dtof_run_mode_e run_mode, dtof_bool_t *is_legal_data)
{
    DTOF_LOG("get ft data multi mode use weak func!\n");

    return DTOF_RET_SUCCESS;
}

DTOF_RET DTOF_WEAK dtof_set_ft_data_to_flash_multi_mode(dtof_uint16_t *ft_data, dtof_uint16_t len, dtof_run_mode_e run_mode)
{
    DTOF_LOG("set ft data multi mode use weak func!\n");

    return DTOF_RET_SUCCESS;
}
