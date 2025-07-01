/*
 * stm32_spi.c
 *
 *  Created on: 2024/9/11
 *      Author: liuzihao
 */
#include <stdlib.h>
#include <string.h>
#include "platform_user_config.h"

#include "base/inc/mos_platform.h"
#include "user/device/device.h"
#include "inc/util.h"
#include "stm32g4xx_hal.h"

static mos_spi_info_t stm32_spi_obj[STM32_SPI_INSTANCES_NBR];
static SPI_HandleTypeDef stm32_spi_handle[STM32_SPI_INSTANCES_NBR];
extern const GPIO_TypeDef *g_gpio_port_base[STM32_NGPIO_PORTS];
extern const uint16_t g_gpio_pin_base[STM32_NGPIO_PINS + 1];
extern DTOF_RET stm32_init_gpio(uint32_t gpio, uint32_t cfgset);
extern DTOF_RET stm32_deinit_gpio(uint32_t gpio);
extern DTOF_RET stm32_write_gpio(uint32_t gpio, uint32_t value);


#define CS_QUICK_TAKE() \
    uint8_t gpioport; \
    uint8_t gpiopin; \
    gpioport = (info->nss_pin & MOS_GPIO_PORT_MASK) >> MOS_GPIO_PORT_SHIFT; \
    gpiopin = (info->nss_pin & MOS_GPIO_PIN_MASK) >> MOS_GPIO_PIN_SHIFT; \
    GPIO_TypeDef *GPIOX = (GPIO_TypeDef *)g_gpio_port_base[gpioport]; \
    ((GPIO_TypeDef *)GPIOX)->BRR = g_gpio_pin_base[gpiopin]; \

#define CS_QUICK_GIVE() \
    ((GPIO_TypeDef *)GPIOX)->BSRR = g_gpio_pin_base[gpiopin]; \


#if 0
uint32_t BaudRatePrescaler  = SPI_BAUDRATEPRESCALER_16;

void spi_change_speed(int spi_id, uint32_t speed) {
    struct mos_spi_info *info = &stm32_spi_obj[spi_id];
    SPI_HandleTypeDef *hspi = (SPI_HandleTypeDef *)info->hspi;

    if (speed == 1) {
        BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
    } else if (speed == 2) {
        BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
    } else if (speed == 3) {
        BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
    } else if (speed == 4) {
        BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
    } else if (speed == 5) {
        BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;
    } else if (speed == 6) {
        BaudRatePrescaler = SPI_BAUDRATEPRESCALER_64;
    } else if (speed == 7) {
        BaudRatePrescaler = SPI_BAUDRATEPRESCALER_128;
    } else if (speed == 8) {
        BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;
    }

    hspi->Init.BaudRatePrescaler = BaudRatePrescaler;

    return;
}

uint32_t spi_get_speed(void) {
    uint32_t speedtype = 0;
    if (BaudRatePrescaler == SPI_BAUDRATEPRESCALER_2) {
        speedtype = 1 ;
    } else if (BaudRatePrescaler == SPI_BAUDRATEPRESCALER_4) {
        speedtype = 2 ;
    } else if (BaudRatePrescaler == SPI_BAUDRATEPRESCALER_8) {
        speedtype = 3 ;
    } else if (BaudRatePrescaler == SPI_BAUDRATEPRESCALER_16) {
        speedtype = 4 ;
    } else if (BaudRatePrescaler == SPI_BAUDRATEPRESCALER_32) {
        speedtype = 5 ;
    } else if (BaudRatePrescaler == SPI_BAUDRATEPRESCALER_64) {
        speedtype = 6 ;
    } else if (BaudRatePrescaler == SPI_BAUDRATEPRESCALER_128) {
        speedtype = 7 ;
    } else if (BaudRatePrescaler == SPI_BAUDRATEPRESCALER_256) {
        speedtype = 8 ;
    }
    return speedtype;
}
#endif


static DTOF_RET stm32_spi_init_info(struct mos_spi_info *info) {
    if (!info) {
        return DTOF_RET_ERROR;
    }
    SPI_HandleTypeDef *hspi = (SPI_HandleTypeDef *)info->hspi;
    uint32_t SPI_APB_CLOCK;
    switch (info->spi_id) {
        case 0: {
            __HAL_RCC_GPIOA_CLK_ENABLE();
            __HAL_RCC_SPI1_CLK_ENABLE();

            stm32_init_gpio(STM32_SPI1_NSS_PIN, STM32_SPI1_NSS_CFG);
            stm32_write_gpio(STM32_SPI1_NSS_PIN, MOS_GPIO_PIN_SET);
            stm32_init_gpio(STM32_SPI1_SCK_PIN, STM32_SPI1_SCK_CFG);
            stm32_init_gpio(STM32_SPI1_MISO_PIN, STM32_SPI1_MISO_CFG);
            stm32_init_gpio(STM32_SPI1_MOSI_PIN, STM32_SPI1_MOSI_CFG);
            hspi->Instance = SPI1;
            info->nss_pin = STM32_SPI1_NSS_PIN;
            /* SPI1 on APB2 */
            SPI_APB_CLOCK = HAL_RCC_GetPCLK2Freq();
            break;
        }
        case 1: {
            __HAL_RCC_SPI2_CLK_ENABLE();
            __HAL_RCC_GPIOB_CLK_ENABLE();

            stm32_init_gpio(STM32_SPI2_NSS_PIN, STM32_SPI2_NSS_CFG);
            stm32_write_gpio(STM32_SPI2_NSS_PIN, MOS_GPIO_PIN_SET);
            stm32_init_gpio(STM32_SPI2_SCK_PIN, STM32_SPI2_SCK_CFG);
            stm32_init_gpio(STM32_SPI2_MISO_PIN, STM32_SPI2_MISO_CFG);
            stm32_init_gpio(STM32_SPI2_MOSI_PIN, STM32_SPI2_MOSI_CFG);
            hspi->Instance = SPI2;
            info->nss_pin = STM32_SPI2_NSS_PIN;
            /* SPI2 on APB1 */
            SPI_APB_CLOCK = HAL_RCC_GetPCLK1Freq();
            break;
        }
        default:
            return DTOF_RET_ERROR;
    }
    /* SPI Config */
    if (info->conf.mode & DTOF_SPI_SLAVE)
    {
        hspi->Init.Mode = SPI_MODE_SLAVE;
    }
    else
    {
        hspi->Init.Mode = SPI_MODE_MASTER;
    }
    hspi->Init.Direction = SPI_DIRECTION_2LINES;

    if (info->conf.data_width == 8)
    {
        hspi->Init.DataSize = SPI_DATASIZE_8BIT;
        hspi->TxXferSize = 8;
        hspi->RxXferSize = 8;
    }
    else if (info->conf.data_width == 16)
    {
        hspi->Init.DataSize = SPI_DATASIZE_16BIT;
    }
    else
    {
        return DTOF_RET_ERROR;
    }

    if (info->conf.mode & DTOF_SPI_CPHA)
    {
        hspi->Init.CLKPhase = SPI_PHASE_2EDGE;
    }
    else
    {
        hspi->Init.CLKPhase = SPI_PHASE_1EDGE;
    }

    if (info->conf.mode & DTOF_SPI_CPOL)
    {
        hspi->Init.CLKPolarity = SPI_POLARITY_HIGH;
    }
    else
    {
        hspi->Init.CLKPolarity = SPI_POLARITY_LOW;
    }
    if (info->conf.mode & DTOF_SPI_NO_CS)
    {
        hspi->Init.NSS = SPI_NSS_SOFT;
    }
    else
    {
        hspi->Init.NSS = SPI_NSS_HARD_OUTPUT;
    }
    if (info->conf.max_hz >= SPI_APB_CLOCK / 2)
    {
        hspi->Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
    }
    else if (info->conf.max_hz >= SPI_APB_CLOCK / 4)
    {
        hspi->Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
    }
    else if (info->conf.max_hz >= SPI_APB_CLOCK / 8)
    {
        hspi->Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
    }
    else if (info->conf.max_hz >= SPI_APB_CLOCK / 16)
    {
        hspi->Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
    }
    else if (info->conf.max_hz >= SPI_APB_CLOCK / 32)
    {
        hspi->Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;
    }
    else if (info->conf.max_hz >= SPI_APB_CLOCK / 64)
    {
        hspi->Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_64;
    }
    else if (info->conf.max_hz >= SPI_APB_CLOCK / 128)
    {
        hspi->Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_128;
    }
    else
    {
        hspi->Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;
    }

    if (info->conf.mode & DTOF_SPI_MSB)
    {
        hspi->Init.FirstBit = SPI_FIRSTBIT_MSB;
    }
    else
    {
        hspi->Init.FirstBit = SPI_FIRSTBIT_LSB;
    }
    hspi->Init.TIMode = SPI_TIMODE_DISABLE;
    hspi->Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi->Init.CRCPolynomial = 7;
    hspi->Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
    hspi->Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
    HAL_SPI_Init(hspi);
    __HAL_SPI_ENABLE(hspi);
    return DTOF_RET_SUCCESS;
}

static uint16_t spi_read_write_u16(struct mos_spi_info *info, uint16_t txdata) {
    SPI_HandleTypeDef *hspi = (SPI_HandleTypeDef *)info->hspi;
    if (!hspi) return DTOF_RET_ERROR;
    while ((hspi->Instance->SR & SPI_FLAG_TXE) == RESET) {
    }
    hspi->Instance->DR = txdata;
    while ((hspi->Instance->SR & SPI_FLAG_RXNE) == RESET) {
    }
    return (uint16_t)hspi->Instance->DR;
}

static int stm32_spi_write_word(int spi_id, uint8_t reg, uint16_t val) {
    struct mos_spi_info *info = &stm32_spi_obj[spi_id];
    uint16_t reg_addr = 0x0000;  // addr:16bits
    reg_addr = (0x00 << 15) | (reg << 3);
    CS_QUICK_TAKE();
    spi_read_write_u16(info, reg_addr);
    spi_read_write_u16(info, val);
    CS_QUICK_GIVE();
    return DTOF_RET_SUCCESS;
}
int stm32_spi_write_block(int spi_id, uint8_t reg, uint8_t *input_buf, uint16_t input_len) {
    struct mos_spi_info *info = &stm32_spi_obj[spi_id];
    uint16_t *input_buf_p = (uint16_t *)input_buf;
    uint16_t reg_addr = 0x0000;
    uint16_t i = 0;
    reg_addr = (0x00 << 15) | (reg << 3);
    CS_QUICK_TAKE();
    spi_read_write_u16(info, reg_addr);
    for (i = 0; i < input_len; i++) {
        spi_read_write_u16(info, *(input_buf_p+i));
    }
    CS_QUICK_GIVE();
    return DTOF_RET_SUCCESS;
}

static int stm32_spi_read_word(int spi_id, uint8_t reg, uint16_t *buf) {
    struct mos_spi_info *info = &stm32_spi_obj[spi_id];
    uint16_t reg_addr = 0x0000;            // addr:16bits
    reg_addr = (0x01 << 15) | (reg << 3);  // wrap addr
    CS_QUICK_TAKE();
    spi_read_write_u16(info, reg_addr);
    *buf = spi_read_write_u16(info, 0xFF);
    CS_QUICK_GIVE();
    return DTOF_RET_SUCCESS;
}

int stm32_spi_read_block(int spi_id, uint8_t reg, uint8_t *output_buf, uint16_t read_len) {
    struct mos_spi_info *info = &stm32_spi_obj[spi_id];
    uint16_t reg_addr = 0x0000;
    uint16_t *output_buf_p = (uint16_t *)output_buf;
    uint16_t i;
    reg_addr = (0x01 << 15) | (reg << 3);  // wrap addr
    CS_QUICK_TAKE();
    spi_read_write_u16(info, reg_addr);
    for (i = 0; i < read_len; i++) {
         *(output_buf_p + i) = spi_read_write_u16(info, 0x0000);
    }
    CS_QUICK_GIVE();
    return DTOF_RET_SUCCESS;
}
static DTOF_RET stm32_spi_configure(int spi_id, void* cfg) {
    struct mos_spi_configuration *configuration = (struct mos_spi_configuration *)cfg;
    if(!configuration) return DTOF_RET_ERROR;

    struct mos_spi_info *info = &stm32_spi_obj[spi_id];
    int ret = 0;

    info->conf = *configuration;
    return DTOF_RET_SUCCESS;
}

DTOF_RET stm32_spi_deinit(int spi_id) {
    if(spi_id >= STM32_SPI_INSTANCES_NBR)
    {
        return DTOF_RET_ERROR;
    }

    DTOF_RET ret = DTOF_RET_SUCCESS;
    struct mos_spi_info *info = &stm32_spi_obj[spi_id];
    SPI_HandleTypeDef *hspi = (SPI_HandleTypeDef *)info->hspi;
    hspi->Instance->CR1 = 0;
    hspi->Instance->CR2 = 0;
    __HAL_SPI_DISABLE(hspi);

    switch (spi_id) {
    case 0: {
        stm32_write_gpio(STM32_SPI1_NSS_PIN, MOS_GPIO_PIN_RESET);
        stm32_write_gpio(STM32_SPI1_SCK_PIN, MOS_GPIO_PIN_RESET);
        stm32_write_gpio(STM32_SPI1_MISO_PIN, MOS_GPIO_PIN_RESET);
        stm32_write_gpio(STM32_SPI1_MOSI_PIN, MOS_GPIO_PIN_RESET);

        stm32_deinit_gpio(STM32_SPI1_NSS_PIN);
        stm32_deinit_gpio(STM32_SPI1_SCK_PIN);
        stm32_deinit_gpio(STM32_SPI1_MISO_PIN);
        stm32_deinit_gpio(STM32_SPI1_MOSI_PIN);
        break;
    }
    case 1: {
        stm32_write_gpio(STM32_SPI2_NSS_PIN, MOS_GPIO_PIN_RESET);
        stm32_write_gpio(STM32_SPI2_SCK_PIN, MOS_GPIO_PIN_RESET);
        stm32_write_gpio(STM32_SPI2_MISO_PIN, MOS_GPIO_PIN_RESET);
        stm32_write_gpio(STM32_SPI2_MOSI_PIN, MOS_GPIO_PIN_RESET);

        stm32_deinit_gpio(STM32_SPI2_NSS_PIN);
        stm32_deinit_gpio(STM32_SPI2_SCK_PIN);
        stm32_deinit_gpio(STM32_SPI2_MISO_PIN);
        stm32_deinit_gpio(STM32_SPI2_MOSI_PIN);
        break;
    }
    default:
        return DTOF_RET_ERROR;
    }

    HAL_SPI_DeInit(hspi);
    memset((void *)hspi, 0, sizeof(SPI_HandleTypeDef));
    info->flags = 0;
    return DTOF_RET_SUCCESS;
}

static dtof_bool_t stm32_spi_device_is_open(struct mos_spi_info *info) {
    return (info->flags & MOS_SPI_FLAG_OPENED) ? DTOF_TRUE : DTOF_FALSE;
}

DTOF_RET stm32_spi_init(int spi_id)
{
    if(spi_id >= STM32_SPI_INSTANCES_NBR)
    {
        return DTOF_RET_ERROR;
    }

    DTOF_RET ret = DTOF_RET_SUCCESS;
    struct mos_spi_info *info = &stm32_spi_obj[spi_id];

    if (stm32_spi_device_is_open(info)) {
        return DTOF_RET_SUCCESS;
    }

    info->spi_id = spi_id;
    info->hspi = &(stm32_spi_handle[spi_id]);
    info->conf.mode = DTOF_SPI_MASTER | DTOF_SPI_MODE_0 | DTOF_SPI_MSB |DTOF_SPI_NO_CS;
    info->conf.data_width = 16;
    // 目前支持的spi速率取决于APB时钟, 目前APB时钟为180MHz, 所以支持的spi速率为: 90MHZ, 45MHZ, 22.5MHZ, 11.25MHZ, 5.625MHZ, 2.8125MHZ, 1.40625MHZ, 0.703125MHZ
    // conf.max_hz 1~8代表上面8个速率
    // info->conf.max_hz = 20 * 1000 * 1000;
    info->conf.max_hz = 5625000;
    // info->conf.max_hz = 22500000;

    ret = stm32_spi_init_info(info);
    if (DTOF_RET_SUCCESS != ret)
    {
        return DTOF_RET_ERROR;
    }

    return ret;
}

DTOF_RET spi_change_speed(int spi_id, uint32_t speed) {
    if(spi_id >= STM32_SPI_INSTANCES_NBR)
    {
        return DTOF_RET_ERROR;
    }
    stm32_spi_deinit(spi_id);

    DTOF_RET ret = DTOF_RET_SUCCESS;
    struct mos_spi_info *info = &stm32_spi_obj[spi_id];

    info->spi_id = spi_id;
    info->hspi = &(stm32_spi_handle[spi_id]);
    info->conf.mode = DTOF_SPI_MASTER | DTOF_SPI_MODE_0 | DTOF_SPI_MSB |DTOF_SPI_NO_CS;
    info->conf.data_width = 16;
    // 目前支持的spi速率取决于APB时钟, 目前APB时钟为180MHz, 所以支持的spi速率为: 90MHZ, 45MHZ, 22.5MHZ, 11.25MHZ, 5.625MHZ, 2.8125MHZ, 1.40625MHZ, 0.703125MHZ
    // conf.max_hz 1~8代表上面8个速率
    info->conf.max_hz = 11250000;
    if(speed == 1){
        info->conf.max_hz = 11250000;
    }
    else if(speed == 2){
        info->conf.max_hz = 5625000;
    }
    else if(speed == 3){
        info->conf.max_hz = 2812500;
    }else if(speed == 4){
        info->conf.max_hz = 1406250;
    }else if(speed == 5){
        info->conf.max_hz = 703125;
    }

    ret = stm32_spi_init_info(info);
    if (DTOF_RET_SUCCESS != ret)
    {
        return DTOF_RET_ERROR;
    }

    return ret;
}

device_driver_ops_t device_spi_driver_ops = {
    .init = stm32_spi_init,
    .deinit = stm32_spi_deinit,
    .write_block = stm32_spi_write_block,
    .read_block = stm32_spi_read_block,
};

