/*
 * board.h
 *
 *  Created on: 2024/9/11
 *      Author: liuzihao
 */

#ifndef __BOARD_H__
#define __BOARD_H__

#include <stm32g4xx.h>
#include "stm32g4xx_hal.h"

#include "inc/dtof_base_type.h"

DTOF_RET hw_board_init(void);
DTOF_RET hw_board_uninit(void);

#endif

