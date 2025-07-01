/*
 * stm32_gpio.c
 *
 *  Created on: 2024/9/11
 *      Author: liuzihao
 */
#include <stdlib.h>

#include "platform_user_config.h"

#include "base/inc/mos_platform.h"
#include "inc/util.h"
#include "user/device/device.h"

// typedef DTOF_RET (*gpio_isr_fn)(int irq, void *context, void *priv);

/* struct for IRQ edges and ISR management */
struct gpio_irq {
    gpio_isr_fn isr;
    void *priv;
    uint8_t flags;
    uint32_t debouncetime; /* debounce time in ms */
    struct timespec debounce_tv;
};

const GPIO_TypeDef *g_gpio_port_base[STM32_NGPIO_PORTS] = {
    STM32_GPIOA_BASE, STM32_GPIOB_BASE, STM32_GPIOC_BASE, STM32_GPIOD_BASE,
    STM32_GPIOE_BASE, STM32_GPIOF_BASE, STM32_GPIOG_BASE
}; /* CONFIG_STM32_STM32G474XX */

const uint16_t g_gpio_pin_base[STM32_NGPIO_PINS + 1] = {
    GPIO_PIN_0,  GPIO_PIN_1,  GPIO_PIN_2,  GPIO_PIN_3,  GPIO_PIN_4,
    GPIO_PIN_5,  GPIO_PIN_6,  GPIO_PIN_7,  GPIO_PIN_8,  GPIO_PIN_9,
    GPIO_PIN_10, GPIO_PIN_11, GPIO_PIN_12, GPIO_PIN_13, GPIO_PIN_14,
    GPIO_PIN_15, GPIO_PIN_All}; /* CONFIG_STM32_STM32G474XX */

const int g_gpio_swline[STM32_NR_SWINT] = {
    STM32_IRQ_EXTI0,
    STM32_IRQ_EXTI1,
    STM32_IRQ_EXTI2,
    STM32_IRQ_EXTI3
};

#define STM32_MAX_PRIOR 16
#define MOS_STM32_PRIO_RES (MOS_TASK_MAX_PRIORITY / STM32_MAX_PRIOR + 1)

int g_PriorMap[STM32_MAX_PRIOR] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
};

#define STM32_PRIORITY(ws_prior) g_PriorMap[(ws_prior) / MOS_STM32_PRIO_RES]


struct gpio_irq g_irqvector[STM32_MAXNR_IRQ + 1];

#define __STM32_PIN(index, gpio, gpio_index) \
    { index, GPIO##gpio##_CLK_ENABLE, GPIO##gpio, GPIO_PIN_##gpio_index }
struct stm32_pin_index {
    int index;
    void (*rcc)(void);
    void *gpio;
    int pin;
};

#ifdef __HAL_RCC_GPIOA_CLK_ENABLE
static void GPIOA_CLK_ENABLE(void) { __HAL_RCC_GPIOA_CLK_ENABLE(); }
#endif
#ifdef __HAL_RCC_GPIOB_CLK_ENABLE
static void GPIOB_CLK_ENABLE(void) { __HAL_RCC_GPIOB_CLK_ENABLE(); }
#endif

#ifdef __HAL_RCC_GPIOC_CLK_ENABLE
static void GPIOC_CLK_ENABLE(void) { __HAL_RCC_GPIOC_CLK_ENABLE(); }
#endif

#ifdef __HAL_RCC_GPIOD_CLK_ENABLE
static void GPIOD_CLK_ENABLE(void) { __HAL_RCC_GPIOD_CLK_ENABLE(); }
#endif

#ifdef __HAL_RCC_GPIOE_CLK_ENABLE
static void GPIOE_CLK_ENABLE(void) { __HAL_RCC_GPIOE_CLK_ENABLE(); }
#endif

#ifdef __HAL_RCC_GPIOF_CLK_ENABLE
static void GPIOF_CLK_ENABLE(void) { __HAL_RCC_GPIOF_CLK_ENABLE(); }
#endif
#ifdef __HAL_RCC_GPIOG_CLK_ENABLE
static void GPIOG_CLK_ENABLE(void) { __HAL_RCC_GPIOG_CLK_ENABLE(); }
#endif

static const struct stm32_pin_index stm32_pins[] = {
    __STM32_PIN(0, A, 0), __STM32_PIN(1, B, 0), __STM32_PIN(2, C, 0),
    __STM32_PIN(3, D, 0), __STM32_PIN(4, E, 0), __STM32_PIN(5, F, 0),
    __STM32_PIN(6, G, 0)};
DTOF_RET irq_unexpected_isr(int irq, void *context, void *priv) {
    return DTOF_RET_ERROR;
}

int gpiopin_2_irq(uint8_t gpiopin) {
    int irq = -1;
    if (gpiopin < 5) {
        irq = gpiopin + STM32_IRQ_EXTI0;  // b4  4+6=10
    } else if (gpiopin < 10) {
        irq = STM32_IRQ_EXTI95;
    } else {
        irq = STM32_IRQ_EXTI1510;
    }
    return irq;
}

int irq_2_gpiopin(int irq) {
    uint8_t gpiopin = 0;
    switch (irq) {
        case STM32_IRQ_EXTI0: {
            gpiopin = 0;
            break;
        }
        case STM32_IRQ_EXTI1: {
            gpiopin = 1;
            break;
        }
        case STM32_IRQ_EXTI2: {
            gpiopin = 2;
            break;
        }
        case STM32_IRQ_EXTI3: {
            gpiopin = 3;
            break;
        }
        case STM32_IRQ_EXTI4: {
            gpiopin = 4;
            break;
        }
        case STM32_IRQ_EXTI95: {
            gpiopin = 5;
            break;
        }
        case STM32_IRQ_EXTI1510: {
            gpiopin = 10;
            break;
        }
        default: {
            break;
        }
    }
    return gpiopin;
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    int irq = 0;
    uint32_t debouncetime = 0;
    switch (GPIO_Pin) {
        case GPIO_PIN_0: {
            irq = STM32_IRQ_EXTI0;
            break;
        }
        case GPIO_PIN_1: {
            irq = STM32_IRQ_EXTI1;
            break;
        }
        case GPIO_PIN_2: {
            irq = STM32_IRQ_EXTI2;
            break;
        }
        case GPIO_PIN_3: {
            irq = STM32_IRQ_EXTI3;
            break;
        }
        case GPIO_PIN_4: {
            irq = STM32_IRQ_EXTI4;
            break;
        }
        default: {
            break;
        }
    }
    gpio_isr_fn vector;

#if STM32_MAXNR_IRQ > 0
    if ((unsigned)irq >= STM32_MAXNR_IRQ || g_irqvector[irq].isr == NULL) {
        vector = irq_unexpected_isr;
    } else {
        vector = g_irqvector[irq].isr;
        debouncetime = g_irqvector[irq].debouncetime;
    }
#else
    vector = irq_unexpected_isr;
#endif
    if (debouncetime > 0) {
        usleep(debouncetime);
    }
    /* Then dispatch to the interrupt handler */
    vector(irq, NULL, g_irqvector[irq].priv);
}

void EXTI0_IRQHandler() { HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0); }

void EXTI1_IRQHandler() { HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_1); }
void EXTI2_IRQHandler() { HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_2); }
void EXTI3_IRQHandler() { HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_3); }
void EXTI4_IRQHandler(void) { HAL_GPIO_EXTI_IRQHandler(TOF_INTERRUPT_PIN); }
void EXTI15_10_IRQHandler_(void) {
    HAL_GPIO_EXTI_IRQHandler(TOF_INTERRUPT_PIN);
}

void irq_initialize(void) {
    int i;
    /* Point all stm32 gpio interrupt vectors to the unexpected interrupt */
    for (i = 0; i < STM32_MAXNR_IRQ; i++) {
        g_irqvector[i].isr = irq_unexpected_isr;
        g_irqvector[i].priv = NULL;
        g_irqvector[i].debouncetime = 0;
    }
}

// /**
//  * @brief
//  *
//  * @param io
//  * @return dtof_bool_t
//  */
// static dtof_bool_t stm32_gpio_is_init(struct gpio *io) {
//     int ret = 0;
//     if (!io) {
//         return DTOF_FALSE;
//     }
//     if (DTOF_SWIT_PIN == io->id) return DTOF_TRUE;
//     return (io->flags & MOS_GPIO_FLAG_OPENED) ? DTOF_TRUE : DTOF_FALSE;
// }

/**
 * @brief
 *
 * @param io
 * @return dtof_bool_t
 */
// static dtof_bool_t stm32_gpio_is_init(struct gpio *io) {
//     int ret = 0;
//     if (!io) {
//         return DTOF_FALSE;
//     }
//     if (DTOF_SWIT_PIN == io->id) return DTOF_TRUE;
//     return (io->flags & MOS_GPIO_FLAG_OPENED) ? DTOF_TRUE : DTOF_FALSE;
// }

/**
 * @brief
 *
 * @param mode
 * @return uint32_t
 */
uint32_t convert_mode(uint8_t mode) {
    uint32_t ret = GPIO_MODE_INPUT;
    switch (mode) {
        case STM32_PIN_MODE_INPUT: {
            ret = GPIO_MODE_INPUT;
            break;
        }
        case STM32_PIN_MODE_OUTPUT_PP: {
            ret = GPIO_MODE_OUTPUT_PP;
            break;
        }
        case STM32_PIN_MODE_OUTPUT_OD: {
            ret = GPIO_MODE_OUTPUT_OD;
            break;
        }
        case STM32_PIN_MODE_AF_PP: {
            ret = GPIO_MODE_AF_PP;
            break;
        }
        case STM32_PIN_MODE_AF_OD: {
            ret = GPIO_MODE_AF_OD;
            break;
        }
        case STM32_PIN_MODE_ANALOG: {
            ret = GPIO_MODE_ANALOG;
            break;
        }
        case STM32_PIN_MODE_IT_RISING: {
            ret = GPIO_MODE_IT_RISING;
            break;
        }
        case STM32_PIN_MODE_IT_FALLING: {
            ret = GPIO_MODE_IT_FALLING;
            break;
        }
        case STM32_PIN_MODE_IT_RISING_FALLING: {
            ret = GPIO_MODE_IT_RISING_FALLING;
            break;
        }
        case STM32_PIN_MODE_IT_LEVEL_HIGH: {
            ret = GPIO_MODE_IT_RISING_FALLING;
            break;
        }
        case STM32_PIN_MODE_IT_LEVEL_LOW: {
            ret = GPIO_MODE_IT_RISING_FALLING;
            break;
        }
        default:
            break;
    }
    return ret;
}

DTOF_RET stm32_enable_gpio_clk(uint32_t gpio, dtof_bool_t eb) {
    int ret = 0;
    uint8_t gpioport;
    uint8_t gpiopin;
    gpioport = (gpio & MOS_GPIO_PORT_MASK) >> MOS_GPIO_PORT_SHIFT;
    DTOF_CHECK_GPIO_PARAM_RETURN(IS_GPIO_PORT_VALID(gpioport),
                                 DTOF_RET_ERROR);
    // switch static const struct stm32_pin_index stm32_pins[] =
    const struct stm32_pin_index *index;
    index = &stm32_pins[gpioport];
    if (index && eb) {
        index->rcc();
    }
    return DTOF_RET_SUCCESS;
}


/**
 * @brief
 *
 * @param io
 * @param cfgset
 * @return DTOF_RET
 */
DTOF_RET stm32_init_gpio(uint32_t gpio, uint32_t cfgset) {
    int ret = 0;
    // if (DTOF_TRUE == stm32_gpio_is_init(io)) return DTOF_RET_SUCCESS;
    uint8_t gpioport;
    uint8_t gpiopin;
    uint8_t pinmode;
    uint8_t pinpull;
    uint8_t pinspeed;
    uint8_t pinalt;
    gpioport = (gpio & MOS_GPIO_PORT_MASK) >> MOS_GPIO_PORT_SHIFT;
    gpiopin = (gpio & MOS_GPIO_PIN_MASK) >> MOS_GPIO_PIN_SHIFT;
    pinmode = (cfgset & STM32_PIN_MODE_MASK) >> STM32_PIN_MODE_SHIFT;
    pinpull = (cfgset & STM32_PIN_PULL_MASK) >> STM32_PIN_PULL_SHIFT;
    pinspeed = (cfgset & STM32_PIN_SPEED_MASK) >> STM32_PIN_SPEED_SHIFT;
    pinalt = (cfgset & STM32_PIN_ALT_MASK) >> STM32_PIN_ALT_SHIFT;

    // 放到gpio init中
    stm32_enable_gpio_clk(gpio,DTOF_TRUE);

    DTOF_CHECK_GPIO_PARAM_RETURN(IS_GPIO_PORT_VALID(gpioport),
                                 DTOF_RET_ERROR);
    DTOF_CHECK_GPIO_PARAM_RETURN(IS_GPIO_PIN_VALID(gpiopin), DTOF_RET_ERROR);
    DTOF_CHECK_GPIO_PARAM_RETURN(
        IS_STM32GPIO_MODE_VALID(cfgset & STM32_PIN_MODE_MASK),
        DTOF_RET_ERROR);
    DTOF_CHECK_GPIO_PARAM_RETURN(
        IS_STM32GPIO_PULL_VALID(cfgset & STM32_PIN_PULL_MASK),
        DTOF_RET_ERROR);
    DTOF_CHECK_GPIO_PARAM_RETURN(
        IS_STM32GPIO_SPEED_VALID(cfgset & STM32_PIN_SPEED_MASK),
        DTOF_RET_ERROR);

    GPIO_TypeDef *GPIOX = (GPIO_TypeDef *)g_gpio_port_base[gpioport];

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = g_gpio_pin_base[gpiopin];
    GPIO_InitStruct.Mode = convert_mode(pinmode);
    GPIO_InitStruct.Pull = pinpull;
    GPIO_InitStruct.Speed = pinspeed;
    GPIO_InitStruct.Alternate = pinalt;
    HAL_GPIO_Init(GPIOX, &GPIO_InitStruct);
    // io->flags |= MOS_GPIO_FLAG_OPENED;
    return DTOF_RET_SUCCESS;
}

/**
 * @brief
 *
 * @param io
 * @return DTOF_RET
 */
DTOF_RET stm32_deinit_gpio(uint32_t gpio) {
    int ret = 0;
    // if (DTOF_FALSE == stm32_gpio_is_init(io)) return DTOF_RET_ERROR;
    uint8_t gpioport;
    uint8_t gpiopin;
    gpioport = (gpio & MOS_GPIO_PORT_MASK) >> MOS_GPIO_PORT_SHIFT;
    gpiopin = (gpio & MOS_GPIO_PIN_MASK) >> MOS_GPIO_PIN_SHIFT;
    GPIO_TypeDef *GPIOX = (GPIO_TypeDef *)g_gpio_port_base[gpioport];
    HAL_GPIO_DeInit(GPIOX, g_gpio_pin_base[gpiopin]);
    // io->flags &= ~MOS_GPIO_FLAG_OPENED;
    return DTOF_RET_SUCCESS;
}

/**
 * @brief
 *
 * @param io
 * @param value
 * @return DTOF_RET
 */
DTOF_RET stm32_read_gpio(uint32_t gpio, uint32_t *value) {
    int ret = 0;
    // if (!io) {
    //     return DTOF_RET_ERROR;
    // }
    // if (DTOF_FALSE == stm32_gpio_is_init(io)) return DTOF_RET_ERROR;
    // uint32_t gpio = io->id;
    uint8_t gpioport;
    uint8_t gpiopin;
    gpioport = (gpio & MOS_GPIO_PORT_MASK) >> MOS_GPIO_PORT_SHIFT;
    gpiopin = (gpio & MOS_GPIO_PIN_MASK) >> MOS_GPIO_PIN_SHIFT;
    GPIO_TypeDef *GPIOX = (GPIO_TypeDef *)g_gpio_port_base[gpioport];
    *value = HAL_GPIO_ReadPin(GPIOX, g_gpio_pin_base[gpiopin]);
    return DTOF_RET_SUCCESS;
}

/**
 * @brief
 *
 * @param io
 * @param value
 * @return DTOF_RET
 */
DTOF_RET stm32_write_gpio(uint32_t gpio, uint32_t value) {
    int ret = 0;
    // if (!io) {
    //     return DTOF_RET_ERROR;
    // }
    // if (DTOF_FALSE == stm32_gpio_is_init(io)) return DTOF_RET_ERROR;
    // uint32_t gpio = io->id;
    uint8_t gpioport;
    uint8_t gpiopin;
    gpioport = (gpio & MOS_GPIO_PORT_MASK) >> MOS_GPIO_PORT_SHIFT;
    gpiopin = (gpio & MOS_GPIO_PIN_MASK) >> MOS_GPIO_PIN_SHIFT;
    GPIO_TypeDef *GPIOX = (GPIO_TypeDef *)g_gpio_port_base[gpioport];
    HAL_GPIO_WritePin(GPIOX, g_gpio_pin_base[gpiopin], value);
    return DTOF_RET_SUCCESS;
}

/**
 * @brief
 *
 * @param io
 * @param priority
 * @param isr
 * @param isr_data
 * @return DTOF_RET
 */
DTOF_RET stm32_irq_attach(uint32_t gpio, int priority,
                                    gpio_isr_fn isr, void *isr_data) {
    int ret = 0;
    // if (DTOF_FALSE == stm32_gpio_is_init(io)) return DTOF_RET_ERROR;
    if (priority >= DTOF_INT_GERATEST_PRIORITY) return DTOF_RET_ERROR;

    uint8_t gpioport;
    uint8_t gpiopin;
    gpioport = (gpio & MOS_GPIO_PORT_MASK) >> MOS_GPIO_PORT_SHIFT;
    gpiopin = (gpio & MOS_GPIO_PIN_MASK) >> MOS_GPIO_PIN_SHIFT;  // 0-15
    GPIO_TypeDef *GPIOX = (GPIO_TypeDef *)g_gpio_port_base[gpioport];
    /* Select the interrupt handler for this EXTI pin */
    // io->irq.isr = isr;
    // io->irq.priv = isr_data;
    int irq = gpiopin_2_irq(gpiopin);
    if (irq < 0) return DTOF_RET_ERROR;
    g_irqvector[irq].isr = isr;
    g_irqvector[irq].priv = isr_data;
    /* EXTI interrupt init*/
    HAL_NVIC_SetPriority(irq, STM32_PRIORITY(priority), 0);
    HAL_NVIC_EnableIRQ(irq);
    return DTOF_RET_SUCCESS;
}

DTOF_RET stm32_irq_enable(uint32_t gpio, dtof_bool_t enabled) {
    int ret = 0;
    // if (DTOF_FALSE == stm32_gpio_is_init(io)) return DTOF_RET_ERROR;
    uint8_t gpioport;
    uint8_t gpiopin;
    gpioport = (gpio & MOS_GPIO_PORT_MASK) >> MOS_GPIO_PORT_SHIFT;
    gpiopin = (gpio & MOS_GPIO_PIN_MASK) >> MOS_GPIO_PIN_SHIFT;  // 0-15
    int irq = gpiopin_2_irq(gpiopin);
    if(enabled == DTOF_TRUE){
        HAL_NVIC_EnableIRQ(irq);
    }else if (enabled == DTOF_FALSE){
        HAL_NVIC_DisableIRQ(irq);
    }else{
        return DTOF_RET_ERROR;
    }

    return DTOF_RET_SUCCESS;
}
DTOF_RET stm32_irq_detach(uint32_t gpio) {
    int ret = 0;
    // if (DTOF_FALSE == stm32_gpio_is_init(io)) return DTOF_RET_ERROR;
    // uint32_t gpio = io->id;
    uint8_t gpioport;
    uint8_t gpiopin;
    gpioport = (gpio & MOS_GPIO_PORT_MASK) >> MOS_GPIO_PORT_SHIFT;
    gpiopin = (gpio & MOS_GPIO_PIN_MASK) >> MOS_GPIO_PIN_SHIFT;  // 0-15
    GPIO_TypeDef *GPIOX = (GPIO_TypeDef *)g_gpio_port_base[gpioport];
    // io->irq.isr = NULL;
    // io->irq.priv = NULL;
    int irq = gpiopin_2_irq(gpiopin);
    if (irq < 0) return DTOF_RET_ERROR;
    g_irqvector[irq].isr = NULL;
    g_irqvector[irq].priv = NULL;
    return DTOF_RET_SUCCESS;
}
// /**
//  * @brief
//  *
//  * @param io
//  * @param trigger
//  * @return DTOF_RET
//  */
// static DTOF_RET stm32_irq_set_trigger_type(struct gpio *io, int trigger) {
//     return DTOF_RET_SUCCESS;
// }

// /**
//  * @brief
//  *
//  * @param io
//  * @return DTOF_RET
//  */
// static DTOF_RET stm32_irq_clear(struct gpio *io) {
//     /* No need clear IRQ because stm32 hal automatically done with code
//      * __HAL_GPIO_EXTI_CLEAR_IT in HAL_GPIO_EXTI_IRQHandler */
//     return DTOF_RET_SUCCESS;
// }

/**
 * @brief
 *
 * @param io
 * @param delay
 * @return DTOF_RET
 */
DTOF_RET stm32_irq_set_debounce(uint32_t gpio, uint16_t delay) {
    int ret = 0;
    // if (DTOF_FALSE == stm32_gpio_is_init(io)) return DTOF_RET_ERROR;
    uint8_t gpiopin;
    gpiopin = (gpio & MOS_GPIO_PIN_MASK) >> MOS_GPIO_PIN_SHIFT;
    int irq = gpiopin_2_irq(gpiopin);
    if (irq < 0) return DTOF_RET_ERROR;
    g_irqvector[irq].debouncetime = delay;
    return DTOF_RET_SUCCESS;
}

/**
 * @brief
 *
 * @param io
 * @param priority
 * @param isr
 * @param isr_data
 * @param irq
 * @return DTOF_RET
 */
DTOF_RET stm32_swit_attach(uint32_t gpio, int priority,
                                     gpio_isr_fn isr, void *isr_data,
                                     int *irq) {
    int ret = 0;
    // if (DTOF_FALSE == stm32_gpio_is_init(io)) return DTOF_RET_ERROR;
    if (DTOF_INT_PRIORITY_GERATEST > priority ||
        DTOF_INT_PRIORITY_BELOW_IDLE < priority)
        return DTOF_RET_ERROR;
    int irqtmp = -1;
    for (int i = 0; i < ARRAY_SIZE(g_irqvector); i++) {
        int findavailable_swline = 0;
        for (int j = 0; j < ARRAY_SIZE(g_gpio_swline); j++) {
            if (i == g_gpio_swline[j] &&
                (NULL == g_irqvector[i].isr ||
                 irq_unexpected_isr == g_irqvector[i].isr)) {
                findavailable_swline = 1;
                break;
            }
        }
        if (findavailable_swline) {
            irqtmp = i;
            break;
        }
    }
    if (irqtmp < 0) return DTOF_RET_ERROR;
    g_irqvector[irqtmp].isr = isr;
    g_irqvector[irqtmp].priv = isr_data;
    /* EXTI interrupt init*/
    uint8_t line = irq_2_gpiopin(irqtmp);
    EXTI->IMR1 |= 1 << line;
    HAL_NVIC_SetPriority(irqtmp, STM32_PRIORITY(priority), 1);
    HAL_NVIC_EnableIRQ(irqtmp);
    *irq = irqtmp;
    return DTOF_RET_SUCCESS;
}

/**
 * @brief
 *
 * @param io
 * @param irq
 * @return DTOF_RET
 */
DTOF_RET stm32_generate_swit(uint32_t gpio, int irq) {
    int ret = 0;
    // if (DTOF_FALSE == stm32_gpio_is_init(io)) return DTOF_RET_ERROR;
    uint8_t line = irq_2_gpiopin(irq);
    EXTI->SWIER1 |= 1 << line;
    return DTOF_RET_SUCCESS;
}

device_driver_gpio_ops_t device_gpio_driver_ops = {
    .init_gpio_fn = stm32_init_gpio,
    .deinit_gpio_fn = stm32_deinit_gpio,
    .read_gpio_fn = stm32_read_gpio,
    .write_gpio_fn = stm32_write_gpio,
    .irq_attach_fn = stm32_irq_attach,
    .irq_enable_fn = stm32_irq_enable,
    .irq_detach_fn = stm32_irq_detach,
    .irq_set_debounce_fn = stm32_irq_set_debounce,
    .swit_attach_fn = stm32_swit_attach,
    .generate_swit_fn = stm32_generate_swit,
    .rcc_fn = stm32_enable_gpio_clk,
};
