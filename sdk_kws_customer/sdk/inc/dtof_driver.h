/*
 * dtof_driver.h
 *
 *  Created on: 2024/8/5
 *      Author: liuzihao
 */

#ifndef _DTOF_DRIVER_H_
#define _DTOF_DRIVER_H_

#include "inc/dtof_common.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define PACK_IO_INTERACTION_DATA(cmd, value) ((0x2 << 14) + (cmd << 8) + value)

DTOF_RET dtof_reg_burst_read(dtof_uint8_t reg_addr, dtof_uint16_t *reg_data_p, dtof_uint16_t len);
DTOF_RET dtof_reg_burst_write(dtof_uint8_t reg_addr, dtof_uint16_t *reg_data_p, dtof_uint16_t len);
DTOF_RET dtof_reg_burst_write_burn(dtof_uint8_t reg_addr, const dtof_uint16_t *reg_data_p, dtof_uint16_t len);
DTOF_RET dtof_io_interaction(dtof_uint16_t cmd, dtof_uint16_t value);
void dtof_set_interrupt_flag(dtof_bool_t flag);
dtof_bool_t dtof_get_interrupt_flag(void);
void dtof_sleep_ms(dtof_uint32_t time);
DTOF_RET dtof_get_ft_data_from_flash(dtof_uint16_t *ft_data, dtof_uint16_t len, dtof_bool_t *is_legal_data);
DTOF_RET dtof_set_ft_data_to_flash(dtof_uint16_t *ft_data, dtof_uint16_t len);

#ifdef __cplusplus
}
#endif

#endif // _DTOF_DRIVER_H_
