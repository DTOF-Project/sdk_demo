/*
 * stm32_uart.c
 *
 *  Created on: 2024/9/11
 *      Author: liuzihao
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "platform_user_config.h"
#include "base/inc/mos_platform.h"
#include "user/device/device.h"
#include "inc/util.h"

extern DTOF_RET stm32_init_gpio(uint32_t gpio, uint32_t cfgset);
extern DTOF_RET stm32_deinit_gpio(uint32_t gpio);

static mos_uart_info_t stm32_uart_obj[STM32_UART_INSTANCES_NBR];
static UART_HandleTypeDef stm32_uart_handle[STM32_UART_INSTANCES_NBR];
static DMA_HandleTypeDef stm32_uart_dma_handle[STM32_UART_INSTANCES_NBR*2];

static void stm32_uart_interupt_rx_cb(struct mos_uart_info *info) {
    if (!info) return;
    UART_HandleTypeDef *huart = (UART_HandleTypeDef *)info->huart;
    unsigned char RevData = (uint8_t)huart->Instance->RDR;
    info->rxbuff[info->uart_in_cnt] = RevData;
    if (++info->uart_in_cnt >= RECEIVE_BUF_LEN) {
        info->uart_in_cnt = 0;
    }
}

static uint8_t stm32_uart_get_byte(struct mos_uart_info *info) {
    if (!info) return 0;
    unsigned char nTemp = info->rxbuff[info->uart_out_cnt];
    if (++info->uart_out_cnt >= RECEIVE_BUF_LEN) {
        info->uart_out_cnt = 0;
    }
    return nTemp;
}

static DTOF_RET stm32_uart_init_info(struct mos_uart_info *info) {
    if (!info) {
        return DTOF_RET_ERROR;
    }
    UART_HandleTypeDef *huart = (UART_HandleTypeDef *)info->huart;
    switch (info->uart_id) {
        case UART0_NBR0: {
            huart->Instance = USART1;
            break;
        }
        case UART0_NBR1: {
            huart->Instance = USART2;
            break;
        }
        default:
            return DTOF_RET_ERROR;
    }
    huart->Init.BaudRate = UART_BAUD_RATE;
    huart->Init.WordLength = UART_WORDLENGTH_8B;
    huart->Init.StopBits = UART_STOPBITS_1;
    huart->Init.Parity = UART_PARITY_NONE;
    huart->Init.Mode = UART_MODE_TX_RX;
    huart->Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart->Init.OverSampling = UART_OVERSAMPLING_16;
    huart->Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    huart->Init.ClockPrescaler = UART_PRESCALER_DIV1;
    huart->AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    if (HAL_UART_Init(huart) != HAL_OK) {
        return DTOF_RET_ERROR;
    }
    if (HAL_UARTEx_SetTxFifoThreshold(huart, UART_TXFIFO_THRESHOLD_1_8) !=
        HAL_OK) {
        return DTOF_RET_ERROR;
    }
    if (HAL_UARTEx_SetRxFifoThreshold(huart, UART_RXFIFO_THRESHOLD_1_8) !=
        HAL_OK) {
        return DTOF_RET_ERROR;
    }
    if (HAL_UARTEx_DisableFifoMode(huart) != HAL_OK) {
        return DTOF_RET_ERROR;
    }
#if !defined( \
    COMMUNICATION_VERIFY)  // the switch maybe defined in keil project file.
    // Adaps_set_uart_interrupt_callback(Adaps_uart_interupt_rx_cb);
    /*##-3- Put UART peripheral in reception process
     * ###########################*/
    if (HAL_UART_Receive_IT(huart, (uint8_t *)&info->uartrxbuff[0], 1) !=
        HAL_OK) {
        return DTOF_RET_ERROR;
    }
#endif
    return DTOF_RET_SUCCESS;
}

void USART1_IRQHandler(void) {
    UART_HandleTypeDef *huart = (UART_HandleTypeDef *)stm32_uart_obj[0].huart;

    if (huart->Instance->ISR & USART_ICR_NCF)  // Monitor for overflow
    {
        huart->Instance->ICR |= USART_ICR_NCF;
    }
    if (huart->Instance->ISR & USART_ICR_ORECF)  // Monitor for overflow
    {
        huart->Instance->ICR |= USART_ICR_ORECF;
    }
    if (huart->Instance->ISR & UART_CLEAR_FEF)  // frame error
    {
        huart->Instance->ICR |= UART_CLEAR_FEF;
    }
    if (huart->Instance->ISR & (0x00000001 << 5))  // Monitor for RX data
    {
        stm32_uart_interupt_rx_cb(&stm32_uart_obj[0]);
    }
}

void USART2_IRQHandler(void) {
    UART_HandleTypeDef *huart = (UART_HandleTypeDef *)stm32_uart_obj[1].huart;

    if (huart->Instance->ISR & USART_ICR_NCF)  // Monitor for overflow
    {
        huart->Instance->ICR |= USART_ICR_NCF;
    }
    if (huart->Instance->ISR & USART_ICR_ORECF)  // Monitor for overflow
    {
        huart->Instance->ICR |= USART_ICR_ORECF;
    }
    if (huart->Instance->ISR & UART_CLEAR_FEF)  // frame error
    {
        huart->Instance->ICR |= UART_CLEAR_FEF;
    }
    if (huart->Instance->ISR & (0x00000001 << 5))  // Monitor for RX data
    {
        stm32_uart_interupt_rx_cb(&stm32_uart_obj[1]);
    }
}
void HAL_UART_ErrorCallback(UART_HandleTypeDef *UartHandle) {}

void HAL_UART_MspInit(UART_HandleTypeDef *uartHandle) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if (uartHandle->Instance == USART1) {
        __HAL_RCC_USART1_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();
        /**USART1 GPIO Configuration
        PA9     ------> USART1_TX
        PA10     ------> USART1_RX
        */

        stm32_init_gpio(STM32_UART1_TX_PIN, STM32_UART1_TX_CFG);
        stm32_init_gpio(STM32_UART1_RX_PIN, STM32_UART1_RX_CFG);

#ifdef ENABLE_UART_DMA
        DMA_HandleTypeDef *hdma_usart_tx =
            (DMA_HandleTypeDef *)stm32_uart_obj[0].hdma_usart_tx;
        DMA_HandleTypeDef *hdma_usart_rx =
            (DMA_HandleTypeDef *)stm32_uart_obj[0].hdma_usart_rx;

        /* USART1 DMA Init */
        /* USART1_TX Init */
        hdma_usart_tx->Instance = DMA2_Channel1;
        hdma_usart_tx->Init.Request = DMA_REQUEST_USART1_TX;
        hdma_usart_tx->Init.Direction = DMA_MEMORY_TO_PERIPH;
        hdma_usart_tx->Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_usart_tx->Init.MemInc = DMA_MINC_ENABLE;
        hdma_usart_tx->Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hdma_usart_tx->Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        hdma_usart_tx->Init.Mode = DMA_NORMAL;
        hdma_usart_tx->Init.Priority = DMA_PRIORITY_VERY_HIGH;
        if (HAL_DMA_Init(hdma_usart_tx) != HAL_OK) {
            // Error_Handler();
        }
        __HAL_LINKDMA(uartHandle, hdmatx, *hdma_usart_tx);

        /* USART1_RX Init */
        hdma_usart_rx->Instance = DMA1_Channel2;
        hdma_usart_rx->Init.Request = DMA_REQUEST_USART1_RX;
        hdma_usart_rx->Init.Direction = DMA_PERIPH_TO_MEMORY;
        hdma_usart_rx->Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_usart_rx->Init.MemInc = DMA_MINC_ENABLE;
        hdma_usart_rx->Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hdma_usart_rx->Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        hdma_usart_rx->Init.Mode = DMA_NORMAL;
        hdma_usart_rx->Init.Priority = DMA_PRIORITY_MEDIUM;
        if (HAL_DMA_Init(hdma_usart_rx) != HAL_OK) {
            // Error_Handler();
        }

        __HAL_LINKDMA(uartHandle, hdmarx, *hdma_usart_rx);
#endif

        /* USART1 interrupt Init */
        HAL_NVIC_SetPriority(USART1_IRQn, 1, 0);
        HAL_NVIC_EnableIRQ(USART1_IRQn);
        /* USER CODE BEGIN USART1_MspInit 1 */

        /* USER CODE END USART1_MspInit 1 */
    } else if (uartHandle->Instance == USART2) {
        __HAL_RCC_USART2_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();
        stm32_init_gpio(STM32_UART2_TX_PIN, STM32_UART2_TX_CFG);
        stm32_init_gpio(STM32_UART2_RX_PIN, STM32_UART2_RX_CFG);

#ifdef ENABLE_UART_DMA
        DMA_HandleTypeDef *hdma_usart_tx =
            (DMA_HandleTypeDef *)stm32_uart_obj[1].hdma_usart_tx;
        DMA_HandleTypeDef *hdma_usart_rx =
            (DMA_HandleTypeDef *)stm32_uart_obj[1].hdma_usart_rx;

        /* USART2 DMA Init */
        /* USART2_TX Init */
        hdma_usart_tx->Instance = DMA2_Channel3;
        hdma_usart_tx->Init.Request = DMA_REQUEST_USART2_TX;
        hdma_usart_tx->Init.Direction = DMA_MEMORY_TO_PERIPH;
        hdma_usart_tx->Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_usart_tx->Init.MemInc = DMA_MINC_ENABLE;
        hdma_usart_tx->Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hdma_usart_tx->Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        hdma_usart_tx->Init.Mode = DMA_NORMAL;
        hdma_usart_tx->Init.Priority = DMA_PRIORITY_VERY_HIGH;
        if (HAL_DMA_Init(hdma_usart_tx) != HAL_OK) {
            // Error_Handler();
        }
        __HAL_LINKDMA(uartHandle, hdmatx, *hdma_usart_tx);

        /* USART1_RX Init */
        hdma_usart_rx->Instance = DMA1_Channel4;
        hdma_usart_rx->Init.Request = DMA_REQUEST_USART2_RX;
        hdma_usart_rx->Init.Direction = DMA_PERIPH_TO_MEMORY;
        hdma_usart_rx->Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_usart_rx->Init.MemInc = DMA_MINC_ENABLE;
        hdma_usart_rx->Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hdma_usart_rx->Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        hdma_usart_rx->Init.Mode = DMA_NORMAL;
        hdma_usart_rx->Init.Priority = DMA_PRIORITY_MEDIUM;
        if (HAL_DMA_Init(hdma_usart_rx) != HAL_OK) {
            // Error_Handler();
        }

        __HAL_LINKDMA(uartHandle, hdmarx, *hdma_usart_rx);
#endif

        /* USART1 interrupt Init */
        HAL_NVIC_SetPriority(USART2_IRQn, 1, 0);
        HAL_NVIC_EnableIRQ(USART2_IRQn);
    }
}

void HAL_UART_MspDeInit(UART_HandleTypeDef *uartHandle) {
    if (uartHandle->Instance == USART1) {
        __HAL_RCC_USART1_CLK_DISABLE();
#ifdef ENABLE_UART_DMA
        /* USART1 DMA DeInit */
        HAL_DMA_DeInit(uartHandle->hdmatx);
        HAL_DMA_DeInit(uartHandle->hdmarx);
#endif

        /* USART1 interrupt Deinit */
        HAL_NVIC_DisableIRQ(USART1_IRQn);
    } else if (uartHandle->Instance == USART2) {
        __HAL_RCC_USART2_CLK_DISABLE();
#ifdef ENABLE_UART_DMA
        HAL_DMA_DeInit(uartHandle->hdmatx);
        HAL_DMA_DeInit(uartHandle->hdmarx);
#endif
        HAL_NVIC_DisableIRQ(USART2_IRQn);
    }
}

int stm32_uart_read(int uart_id, void *buf, int nbyte) {
    struct mos_uart_info *info = &stm32_uart_obj[uart_id];
    int readlen = 0;
    while (info->uart_out_cnt != info->uart_in_cnt) {
        uint8_t utmp = stm32_uart_get_byte(info);
        memcpy(buf + readlen, &utmp, sizeof(utmp));
        if (++readlen >= nbyte) {
            break;
        }
    }
    return readlen;
}

int stm32_uart_write(int uart_id, void *buf, int nbyte) {
    struct mos_uart_info *info = &stm32_uart_obj[uart_id];
    return HAL_UART_Transmit(info->huart, (uint8_t *)buf, nbyte, HAL_MAX_DELAY);
}

static dtof_bool_t stm32_uart_device_is_open(struct mos_uart_info *info) {
    return (info->flags & MOS_UART_FLAG_OPENED) ? DTOF_TRUE : DTOF_FALSE;
}

DTOF_RET stm32_uart_init(int uart_id)
{
    if(uart_id >= STM32_UART_INSTANCES_NBR)
    {
        return DTOF_RET_ERROR;
    }

    int ret = 0;
    struct mos_uart_info *info = &stm32_uart_obj[uart_id];

    if (stm32_uart_device_is_open(info)) {
        return DTOF_RET_SUCCESS;
    }

    info->uart_id = uart_id;
    info->huart = &(stm32_uart_handle[uart_id]);

    info->hdma_usart_tx = &(stm32_uart_dma_handle[uart_id * 2]);
    info->hdma_usart_rx = &(stm32_uart_dma_handle[uart_id * 2 + 1]);

    ret = stm32_uart_init_info(info);
    if (DTOF_RET_SUCCESS != ret) return DTOF_RET_ERROR;
    info->flags |= MOS_UART_FLAG_OPENED;
    return DTOF_RET_SUCCESS;
}

static DTOF_RET stm32_uart_deinit(int uart_id)
{
    if(uart_id >= STM32_UART_INSTANCES_NBR)
    {
        return DTOF_RET_ERROR;
    }

    struct mos_uart_info *info = &stm32_uart_obj[uart_id];

    UART_HandleTypeDef *huart = (UART_HandleTypeDef *)info->huart;
    HAL_UART_DeInit(huart);
    switch (uart_id) {
        case UART0_NBR0: {
            stm32_deinit_gpio(STM32_UART1_TX_PIN);
            stm32_deinit_gpio(STM32_UART1_RX_PIN);
            break;
        }
        case UART0_NBR1: {
            stm32_deinit_gpio(STM32_UART2_TX_PIN);
            stm32_deinit_gpio(STM32_UART2_RX_PIN);
            break;
        }
        default:
            return DTOF_RET_ERROR;
    }

    info->flags = 0;
    return DTOF_RET_SUCCESS;
}

int fputc(int ch, FILE *f) {
    struct mos_uart_info *info = &stm32_uart_obj[0];
    HAL_UART_Transmit(info->huart, (uint8_t *)&ch, 1, HAL_MAX_DELAY);  // 将字符发送到串口
    return ch;
}

device_driver_ops_t device_uart_driver_ops = {
    .init = stm32_uart_init,
    .deinit = stm32_uart_deinit,
    .write = stm32_uart_write,
    .read = stm32_uart_read,
};


