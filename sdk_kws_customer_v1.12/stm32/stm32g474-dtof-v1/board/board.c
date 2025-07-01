/*
 * board.c
 *
 *  Created on: 2024/9/11
 *      Author: liuzihao
 */
#include "board.h"
#include "platform_user_config.h"
#include "inc/util.h"
#include "inc/dtof_base_type.h"
#include "fatfs/ff.h"
#include "ffconf.h"
#include "fatfs/ff.h"

extern void irq_initialize(void);

FATFS fatfs;

#ifdef HAVE_HIRES_TIMER
static TIM_HandleTypeDef htim16;
volatile uint32_t s_trig_timer_count = 0;
volatile uint32_t s_trig_timer_cnt = 0;

void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *tim_baseHandle) {
    if (tim_baseHandle->Instance == TIM16) {
        __HAL_RCC_TIM16_CLK_ENABLE();
        HAL_NVIC_SetPriority(TIM1_UP_TIM16_IRQn, 5, 0);
        HAL_NVIC_EnableIRQ(TIM1_UP_TIM16_IRQn);
    }
}

void TIM1_UP_TIM16_IRQHandler(void) {
    s_trig_timer_count++;
    s_trig_timer_cnt = __HAL_TIM_GET_COUNTER(&htim16);
    __HAL_TIM_CLEAR_IT(&htim16, TIM_IT_UPDATE);
}

// period: uint us
DTOF_RET dtof_init_trig_timer(void) {
    /* USER CODE BEGIN TIM16_Init 0 */
    TIM_MasterConfigTypeDef sMasterConfig = {0};
    htim16.Instance = TIM16;
    htim16.Init.Prescaler =
        (SystemCoreClock / 1000000) - 1;  // 1us counter 179 即1us数一次
    htim16.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim16.Init.Period = 1000 - 1;  //定时1ms，1ms=1us*1000, 预装载值就是1000-1
    htim16.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim16.Init.RepetitionCounter = 1;
    htim16.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    if (HAL_TIM_Base_Init(&htim16) != HAL_OK) {
        return DTOF_RET_ERROR;
    }
    /* USER CODE BEGIN TIM16_Init 2 */
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim16, &sMasterConfig) !=
        HAL_OK) {
        return DTOF_RET_ERROR;
    }
    __HAL_TIM_ENABLE_IT(&htim16, TIM_IT_UPDATE);
    HAL_TIM_Base_Start_IT(&htim16);
    return DTOF_RET_SUCCESS;
}

uint64_t dtof_get_trig_timer_count(void) {
    return __HAL_TIM_GET_COUNTER(&htim16);
}
#endif

void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    /** Configure the main internal regulator output voltage
     */
    HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);
    /** Initializes the RCC Oscillators according to the specified parameters
     * in the RCC_OscInitTypeDef structure.
     */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV1;
    RCC_OscInitStruct.PLL.PLLN = 45;  // 40;             // 30; systemclk=4*PLLN
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV3;  // 120
    RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;  // 180
    RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;  // 180 *
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        // Error_Handler();
    }
    /** Initializes the CPU, AHB and APB buses clocks
     */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                  RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK) {
        // Error_Handler();
    }
}

DTOF_RET file_system_init() {
    uint8_t working_buf[SECTOR_SIZE];
    FRESULT fr;
    FATFS *fatff = 0;
    DWORD len = 0;
    fr = f_mount(&fatfs, "0:", 0);
    fr = f_getfree("0:", &len, &fatff);
    if (fr != FR_OK) {
        fr = f_mkfs("0:", FM_FAT, 0, working_buf, SECTOR_SIZE);
        if (FR_OK != fr) {
            return DTOF_RET_ERROR;
        }
        fr = f_mount(NULL, "0:", 0);
        if (FR_OK != fr) {
            return DTOF_RET_ERROR;
        }
        fr = f_mount(&fatfs, "0:", 0);
        if (FR_OK != fr) {
            return DTOF_RET_ERROR;
        }
    }
    return DTOF_RET_SUCCESS;
}

DTOF_RET hw_board_init(void)
{
    HAL_Init();
    SystemClock_Config();

#ifdef HAVE_HIRES_TIMER
    dtof_init_trig_timer();
#endif

    irq_initialize();
    return DTOF_RET_SUCCESS;
}

DTOF_RET hw_board_uninit(void)
{
    return DTOF_RET_SUCCESS;
}























