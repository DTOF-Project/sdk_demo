/*
 * stm32_iic.c
 *
 *  Created on: 2024/9/11
 *      Author: liuzihao
 */
#include <stdlib.h>
#include <string.h>
#include "platform_user_config.h"

#include "base/inc/mos_platform.h"
#include "inc/util.h"
#include "user/device/device.h"

extern DTOF_RET stm32_init_gpio(uint32_t gpio, uint32_t cfgset);
extern DTOF_RET stm32_deinit_gpio(uint32_t gpio);

static mos_i2c_info_t stm32_i2c_obj[STM32_I2C_INSTANCES_NBR];
static I2C_HandleTypeDef stm32_i2c_handle[STM32_I2C_INSTANCES_NBR];

/**
 * @brief
 *
 */
void HAL_MspInit(void) {
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_RCC_PWR_CLK_ENABLE();
    HAL_PWREx_DisableUCPDDeadBattery();
}

/**
 * @brief I2C MSP Initialization
 * This function configures the hardware resources used in this example
 * @param hi2c: I2C handle pointer
 * @retval None
 */
void HAL_I2C_MspInit(I2C_HandleTypeDef *hi2c) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};
    if (hi2c->Instance == I2C1) {
        PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_I2C1;
        PeriphClkInit.I2c1ClockSelection = RCC_I2C1CLKSOURCE_PCLK1;
        if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
            // Error_Handler();
        }
        __HAL_RCC_GPIOA_CLK_ENABLE();
        __HAL_RCC_GPIOB_CLK_ENABLE();
        /**I2C1 GPIO Configuration
        PA15     ------> I2C1_SCL
        PB9     ------> I2C1_SDA
        */
        stm32_init_gpio(STM32_I2C1_SCL_PIN, STM32_I2C1_SCL_CFG);
        stm32_init_gpio(STM32_I2C1_SDA_PIN, STM32_I2C1_SDA_CFG);
        /* Peripheral clock enable */
        __HAL_RCC_I2C1_CLK_ENABLE();
    }
}
static void i2c_delay()
{
  int x = 20; // ref 33 write 769K  read 990K 都正常

  while (x--)
    ;
}
void reset_iic_io(I2C_HandleTypeDef *hi2c){
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    /**I2C1 GPIO Configuration
    PA15     ------> I2C1_SCL
    PB9     ------> I2C1_SDA
    */
    GPIO_InitStruct.Pin = GPIO_PIN_15;
    GPIO_InitStruct.Mode = MODE_OUTPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    GPIO_InitStruct.Pin = GPIO_PIN_9;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    ((GPIO_TypeDef *)GPIOB)->ODR |= GPIO_PIN_9;
    ((GPIO_TypeDef *)GPIOA)->ODR |= GPIO_PIN_15;
    for (int i = 0; i < 9; i++)
    {
        i2c_delay();
        ((GPIO_TypeDef *)GPIOA)->ODR &= ~GPIO_PIN_15;
        i2c_delay();
        ((GPIO_TypeDef *)GPIOA)->ODR |= GPIO_PIN_15;
    }
    i2c_delay();
    ((GPIO_TypeDef *)GPIOB)->ODR |= GPIO_PIN_9;
    ((GPIO_TypeDef *)GPIOA)->ODR |= GPIO_PIN_15;
}
/**
 * @brief I2C MSP De-Initialization
 * This function freeze the hardware resources used in this example
 * @param hi2c: I2C handle pointer
 * @retval None
 */
void HAL_I2C_MspDeInit(I2C_HandleTypeDef *hi2c) {
    if (hi2c->Instance == I2C1) {
        hi2c->Instance->CR1 &= ~(I2C_CR1_PE);
        __HAL_RCC_I2C1_CLK_DISABLE();
        stm32_deinit_gpio(STM32_I2C1_SCL_PIN);
        stm32_deinit_gpio(STM32_I2C1_SDA_PIN);
        reset_iic_io(hi2c);
    }
}

struct mos_i2c_info *infostatic = 0 ;

uint32_t timingvalue = 0;

static DTOF_RET stm32_i2c_init_info(struct mos_i2c_info *info) {
    if (infostatic == 0) {
        infostatic = info;
    }
    I2C_HandleTypeDef *hi2c = (I2C_HandleTypeDef *)info->hi2c;

    switch (info->iic_id) {
        case IIC_NBR0: {
            __HAL_RCC_I2C1_CLK_ENABLE();
            __HAL_RCC_DMA1_CLK_ENABLE();
            __HAL_RCC_GPIOA_CLK_ENABLE();
            __HAL_RCC_GPIOB_CLK_ENABLE();
            hi2c->Instance = I2C1;
            break;
        }
        case IIC_NBR1: {
            // info->hi2c.Instance = I2C2;
            return DTOF_RET_ERROR;
            break;
        }
        default:
            return DTOF_RET_ERROR;
    }

    switch (info->conf.mode) {
        case DTOF_I2C_MODE_STANDARD: {
            hi2c->Init.Timing = 0x808F6061;//Speed:100K
            break;
        }
        case DTOF_I2C_MODE_FAST: {
            hi2c->Init.Timing = 0x404F2828;  // Speed:399.2K
            break;
        }
        case DTOF_I2C_MODE_FASTPLUS: {
            hi2c->Init.Timing = 0x202F3030;//Speed:1M
            break;
        }
        default:
            hi2c->Init.Timing = 0x404F2828;  // Speed:399.2K
            return DTOF_RET_ERROR;
    }

    // hi2c1.Init.Timing = 0x101B5A5A;//Speed:376K 进低功耗没有问题
    // hi2c1.Init.Timing = 0x101F3535;//Speed:634K 进低功耗没有问题
    // hi2c1.Init.Timing = 0x101F3333; // Speed:655K 进低功耗没有问题 有问题
    // hi2c1.Init.Timing = 0x101F3232;//Speed:666.666K 进低功耗有问题，
    // hi2c1.Init.Timing = 0x101F3030;//Speed:689K 进低功耗有问题
    // hi2c1.Init.Timing = 0x404F1111;//Speed:704K
    // hi2c1.Init.Timing = 0x808F1111;//Speed:385K
    // hi2c1.Init.Timing = 0x808F6061;//Speed:100K
    // hi2c1.Init.Timing = 0x000F3535;//Speed:1M
    // hi2c1.Init.Timing = 0x000F2020;//speed 1.4M
    // hi2c1.Init.Timing = 0x202F1515;//Speed:1M
    // hi2c1.Init.Timing = 0x202F3030;//Speed:1M
    // hi2c1.Init.Timing = 0x202F2121; // Speed:666.666K 抓波的配置
    // hi2c1.Init.Timing = 0x202F2222; // Speed:650K
    // 进低功耗有问题，有问题，可以运行几分钟 hi2c1.Init.Timing = 0x202F2424; //
    // Speed:634K 运行一个小时没问题 hi2c1.Init.Timing = 0x202F4343;//Speed:400K

    hi2c->Init.Timing = 0x404F2828;  // Speed:399.2K
//    hi2c->Init.Timing = 0x000f4040;  // Speed:1m
    // update timing
    if (timingvalue != 0) {
        hi2c->Init.Timing = timingvalue;
    }
    if (0 == timingvalue) {
        timingvalue = hi2c->Init.Timing;
    }

    hi2c->Init.OwnAddress1 = 0;
    hi2c->Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c->Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c->Init.OwnAddress2 = 0;
    hi2c->Init.OwnAddress2Masks = I2C_OA2_NOMASK;
    hi2c->Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c->Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    if (HAL_I2C_Init(hi2c) != HAL_OK) {
        // Error_Handler();
        return DTOF_RET_ERROR;
    }

    /** Configure Analogue filter
     */
    if (HAL_I2CEx_ConfigAnalogFilter(hi2c, I2C_ANALOGFILTER_ENABLE) != HAL_OK) {
        // Error_Handler();
        return DTOF_RET_ERROR;
    }

    /** Configure Digital filter
     */
    if (HAL_I2CEx_ConfigDigitalFilter(hi2c, 0) != HAL_OK) {
        // Error_Handler();
        return DTOF_RET_ERROR;
    }

    /** I2C Fast mode Plus enable
     */
    // __HAL_SYSCFG_FASTMODEPLUS_ENABLE(I2C_FASTMODEPLUS_I2C1);
    return DTOF_RET_SUCCESS;
}


uint32_t dtof_i2c_timing_set(uint32_t flag) {
    // deinit
    timingvalue  = flag;

    if (infostatic) {
        I2C_HandleTypeDef *hi2c = (I2C_HandleTypeDef *)infostatic->hi2c;
        HAL_I2C_MspDeInit(hi2c);
        memset((void *)hi2c, 0, sizeof(I2C_HandleTypeDef));
        stm32_i2c_init_info(infostatic);
    }

    return flag;
}

uint32_t dtof_i2c_timing_get() {
    return timingvalue;
}


//#define swap16bit_big_little(val) ((((val)&0x00FF) << 8) | ((val)&0xFF00) >> 8)
static uint16_t DevAddress = ADAPS_ADDR_W;
// static void stm32_i2c_address_set(struct device *dev, uint16_t address) {
//     uint16_t tmp_address = address & 0xff;
//     tmp_address |= (1 << 7);
//     tmp_address = tmp_address << 8;
//     stm32_i2c_write_word(dev, 0x01, tmp_address);
//     DevAddress = (address & 0xff) << 1;


// }


// static uint16_t stm32_i2c_address_get(void) {
//     return DevAddress >> 1;
// }

static inline void i2c_sw_reset(I2C_HandleTypeDef *hi2c)
{
    /*  SW reset procedure:
     *  PE must be kept low during at least 3 APB clock cycles
     *  in order to perform the software reset.
     *  This is ensured by writing the following software sequence:
     *  - Write PE=0
     *  - Check PE=0
     *  - Write PE=1.
     */
    hi2c->Instance->CR1 &=  ~I2C_CR1_PE;
    while (hi2c->Instance->CR1 & I2C_CR1_PE);
    hi2c->Instance->CR1 |=  I2C_CR1_PE;
}

int stm32_i2c_write_word(int iic_id, uint8_t reg, uint16_t val) {
    struct mos_i2c_info *info = &stm32_i2c_obj[iic_id];
    int trycount = 0;
    while(trycount++<3){
        int ret = HAL_I2C_Mem_Write(info->hi2c, DevAddress, reg, 1, (uint8_t *)&val, 2, 2);
        if(HAL_OK!=ret){
            i2c_sw_reset(info->hi2c);
        }else{
            return DTOF_RET_SUCCESS;
        }
    }
    return DTOF_RET_ERROR;
}

int stm32_i2c_write_block(int iic_id, uint8_t reg,
                                 uint8_t *input_buf, uint16_t input_len) {
    struct mos_i2c_info *info = &stm32_i2c_obj[iic_id];
    int trycount = 0;
    while(trycount++<3){
        int ret = HAL_I2C_Mem_Write(info->hi2c, DevAddress, reg, 1, (uint8_t *)input_buf,
                  input_len * 2, input_len);
        if(HAL_OK!=ret){
            i2c_sw_reset(info->hi2c);
        }else{
            return DTOF_RET_SUCCESS;
        }
    }

    return DTOF_RET_ERROR;
}

int stm32_i2c_read_word(int iic_id, uint8_t reg, uint16_t *buf) {
    struct mos_i2c_info *info = &stm32_i2c_obj[iic_id];
    int trycount = 0;
    while(trycount++<3){
        int ret = HAL_I2C_Mem_Read(info->hi2c, DevAddress, reg, 1, (uint8_t *)buf, 2, 2);
        if(HAL_OK!=ret){
            i2c_sw_reset(info->hi2c);
        }else{
            return DTOF_RET_SUCCESS;
        }
    }
    return DTOF_RET_ERROR;
}

int stm32_i2c_read_block(int iic_id, uint8_t reg,
                                uint8_t *output_buf, uint16_t read_len) {
    struct mos_i2c_info *info = &stm32_i2c_obj[iic_id];
    int trycount = 0;
    while(trycount++<3){
        int ret = HAL_I2C_Mem_Read(info->hi2c, DevAddress, reg, 1,
                        (uint8_t *)output_buf, read_len * 2, read_len);
        if(HAL_OK!=ret){
            i2c_sw_reset(info->hi2c);
        }else{
            return DTOF_RET_SUCCESS;
        }
    }
    return DTOF_RET_ERROR;
}

DTOF_RET stm32_ioctl(int cmd, void *arg)
{
    switch (cmd)
    {
    case DTOF_RETRIEVE_DEV_ADDRESS:
    {
        // TODO: @liuzihao 待实现, 不使用dev
        // *((uint16_t *)arg) = stm32_i2c_address_get();
        break;
    }
    case DTOF_OPTION_DEV_ADDRESS:
    {
        // TODO: @liuzihao 待实现, 不使用dev
        // stm32_i2c_address_set(dev, *((uint16_t *)arg));
        break;
    }
    case DTOF_RETRIEVE_DEV_SPEED:
    {
        uint32_t rtimingvalue = dtof_i2c_timing_get();
        if (0x808F6061 == rtimingvalue) {
            *((uint16_t *)arg) = 100;
        } else if (0x404F2828 == rtimingvalue) {
            *((uint16_t *)arg) = 400;
        } else if (0x202F3030 == rtimingvalue) {
            *((uint16_t *)arg) = 1000;
        } else {
            *((uint16_t *)arg) = 0;
        }
        break;
    }
    case DTOF_OPTION_DEV_SPEED:
    {
        if (infostatic)
        {
            uint32_t wtimingvalue = 0x808F6061;
            if (100 >= *((uint16_t *)arg))
            {
                wtimingvalue = 0x808F6061;
                *(uint16_t *)arg = 100;
                infostatic->conf.mode = DTOF_I2C_MODE_STANDARD;
            }
            else if (400 >= *((uint16_t *)arg))
            {
                wtimingvalue = 0x404F2828;
                *(uint16_t *)arg = 400;
                infostatic->conf.mode =DTOF_I2C_MODE_FAST;
            }
            else
            {
                wtimingvalue = 0x202F1515;
                *(uint16_t *)arg = 1000;
                infostatic->conf.mode = DTOF_I2C_MODE_FASTPLUS;
            }
            dtof_i2c_timing_set(wtimingvalue);
        }

    }

        break;
    default:
    {
        return DTOF_RET_ERROR;
    }
    }
    return DTOF_RET_SUCCESS;
}

DTOF_RET stm32_i2c_deinit(int iic_id)
{
    if(iic_id >= STM32_I2C_INSTANCES_NBR)
    {
        return DTOF_RET_ERROR;
    }

    DTOF_RET ret = DTOF_RET_SUCCESS;
    struct mos_i2c_info *info = &stm32_i2c_obj[iic_id];
    I2C_HandleTypeDef *hi2c = (I2C_HandleTypeDef *)info->hi2c;

    HAL_I2C_MspDeInit(hi2c);
    memset((void *)hi2c, 0, sizeof(I2C_HandleTypeDef));
    info->flags = 0;
    return ret;
}

static dtof_bool_t stm32_i2c_device_is_open(struct mos_i2c_info *info) {
    return (info->flags & MOS_I2C_FLAG_OPENED) ? DTOF_TRUE : DTOF_FALSE;
}

DTOF_RET stm32_i2c_init(int iic_id)
{
    if(iic_id >= STM32_I2C_INSTANCES_NBR)
    {
        return DTOF_RET_ERROR;
    }

    DTOF_RET ret = DTOF_RET_SUCCESS;
    struct mos_i2c_info *info = &stm32_i2c_obj[iic_id];

    if (stm32_i2c_device_is_open(info)) {
        return DTOF_RET_SUCCESS;
    }

    info->iic_id = iic_id;
    info->hi2c = &(stm32_i2c_handle[iic_id]);
    info->conf.mode = DTOF_I2C_MODE_FAST;

    ret = stm32_i2c_init_info(info);
    if (DTOF_RET_SUCCESS != ret)
    {
        return DTOF_RET_ERROR;
    }

    return ret;
}

device_driver_ops_t device_iic_driver_ops = {
    .init = stm32_i2c_init,
    .deinit = stm32_i2c_deinit,
    .write_block = stm32_i2c_write_block,
    .read_block = stm32_i2c_read_block,
    // TODO: @liuzihao stm32_ioctl param need iic id
    // .ioctl = stm32_ioctl,
};


