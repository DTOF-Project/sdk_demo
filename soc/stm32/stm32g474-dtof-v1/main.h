/*
 * main.h
 *
 *  Created on: 2024/9/11
 *      Author: liuzihao
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef PLATFORM_STM32_INCLUDE_MAIN_H
#define PLATFORM_STM32_INCLUDE_MAIN_H

/* Includes ------------------------------------------------------------------*/
#include "stm32g4xx_hal.h"
//#include "stm32g474e_eval.h"

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */

int dtof_reg_burst_write(uint8_t reg_addr, uint16_t *reg_data_p, uint16_t len);
int dtof_reg_burst_read(uint8_t reg_addr, uint16_t *reg_data_p, uint16_t len);

#endif /* PLATFORM_STM32_INCLUDE_MAIN_H */
