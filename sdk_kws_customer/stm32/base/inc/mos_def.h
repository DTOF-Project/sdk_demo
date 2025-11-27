/*
 * mos_def.h
 *
 *  Created on: 2024/9/11
 *      Author: liuzihao
 */
#ifndef DRIVERS_BASE_INCLUDE_MOS_DEF_H_
#define DRIVERS_BASE_INCLUDE_MOS_DEF_H_

#include "inc/dtof_base_type.h"
// #include "include/list.h"
#ifndef SRC_CUSTOMER_DSC_CUSTOMER_H_
#define DEVICE_TYPE_I2C "i2c"
#define DEVICE_NAME_I2C0 "/dev/i2c-0"
#define MOS_I2C_FLAG_OPENED BIT(0)

#define DEVICE_TYPE_SPI "spi"
#define DEVICE_NAME_SPI0 "/dev/spi-0"
#define DEVICE_NAME_SPI1 "/dev/spi-1"
#define MOS_SPI_FLAG_OPENED BIT(0)
#define MOS_UART_FLAG_OPENED BIT(0)
#define USART_ICR_NCF (1 << 2)
#define ENABLE_UART_DMA

#define MOS_RDONLY 00000000
#define MOS_WRONLY 00000001
#define MOS_RDWR 00000002
#endif  // SRC_CUSTOMER_DSC_CUSTOMER_H_
// add for UT
#if !defined(__linux__) && !defined(WIN32)
struct timespec {
    time_t tv_sec;
    uint64_t tv_nsec;
};
struct timeval
{
  time_t tv_sec;
  long tv_usec;
};
#endif   //  __linux__
#define DEVICE_TYPE_UART "uart"
#define DEVICE_NAME_COMM_UART PLATFORM_COMM_UART
#define DEVICE_NAME_DEBUG_UART PLATFORM_DEBUG_UART
#define DEVICE_NAME_UART0 "/dev/uart-0"
#define DEVICE_NAME_UART1 "/dev/uart-1"
#define DEVICE_NAME_UART5 "/dev/uart-5"

#define DEVICE_TYPE_FLASH "flash"
#define DEVICE_NAME_FLASH "/dev/flash-0"

#define DEVICE_TYPE_ADC "adc"
#define DEVICE_NAME_ADC0 "/dev/adc-0"
#define DEVICE_NAME_ADC1 "/dev/adc-1"

#define DEVICE_TYPE_DAC "dac"
#define DEVICE_NAME_DAC0 "/dev/dac-0"


#define MOS_FLASH_FLAG_OPENED BIT(0)

#define MOS_READ_FLAG BIT(1)
#define MOS_WRITE_FLAG BIT(2)

#define DEVICE_TYPE_GPIO "gpio"
#define MOS_GPIO_FLAG_OPENED BIT(0)
#define MOS_GPIO_FLAG_CLOSED (0)
#define MOS_GPIO_PULL_UP   (1)
#define MOS_GPIO_PULL_DOWN (0)

#define MOS_GPIO_PORT_SHIFT 4 /* Bit 4-7:  端口组 */
#define MOS_GPIO_PORT_MASK (7 << MOS_GPIO_PORT_SHIFT)
#define MOS_GPIO_PORTA (0 << MOS_GPIO_PORT_SHIFT) /*   GPIOA */
#define MOS_GPIO_PORTB (1 << MOS_GPIO_PORT_SHIFT) /*   GPIOB */
#define MOS_GPIO_PORTC (2 << MOS_GPIO_PORT_SHIFT) /*   GPIOC */
#define MOS_GPIO_PORTD (3 << MOS_GPIO_PORT_SHIFT) /*   GPIOD */
#define MOS_GPIO_PORTE (4 << MOS_GPIO_PORT_SHIFT) /*   GPIOE */
#define MOS_GPIO_PORTF (5 << MOS_GPIO_PORT_SHIFT) /*   GPIOF */
#define MOS_GPIO_PORTG (6 << MOS_GPIO_PORT_SHIFT) /*   GPIOG */
#define MOS_GPIO_PORTH (7 << MOS_GPIO_PORT_SHIFT) /*   GPIOH */

#define MOS_GPIO_PIN_SHIFT 0 /* Bits 0-3: PIN脚: 0-15 */
#define MOS_GPIO_PIN_MASK (15 << MOS_GPIO_PIN_SHIFT)
#define MOS_GPIO_PIN0 (0 << MOS_GPIO_PIN_SHIFT)
#define MOS_GPIO_PIN1 (1 << MOS_GPIO_PIN_SHIFT)
#define MOS_GPIO_PIN2 (2 << MOS_GPIO_PIN_SHIFT)
#define MOS_GPIO_PIN3 (3 << MOS_GPIO_PIN_SHIFT)
#define MOS_GPIO_PIN4 (4 << MOS_GPIO_PIN_SHIFT)
#define MOS_GPIO_PIN5 (5 << MOS_GPIO_PIN_SHIFT)
#define MOS_GPIO_PIN6 (6 << MOS_GPIO_PIN_SHIFT)
#define MOS_GPIO_PIN7 (7 << MOS_GPIO_PIN_SHIFT)
#define MOS_GPIO_PIN8 (8 << MOS_GPIO_PIN_SHIFT)
#define MOS_GPIO_PIN9 (9 << MOS_GPIO_PIN_SHIFT)
#define MOS_GPIO_PIN10 (10 << MOS_GPIO_PIN_SHIFT)
#define MOS_GPIO_PIN11 (11 << MOS_GPIO_PIN_SHIFT)
#define MOS_GPIO_PIN12 (12 << MOS_GPIO_PIN_SHIFT)
#define MOS_GPIO_PIN13 (13 << MOS_GPIO_PIN_SHIFT)
#define MOS_GPIO_PIN14 (14 << MOS_GPIO_PIN_SHIFT)
#define MOS_GPIO_PIN15 (15 << MOS_GPIO_PIN_SHIFT)

//配置LED2
#define DTOF_LED2_PIN PLATFORM_LED2_PIN
#define DTOF_LED2_CFG PLATFORM_LED2_CFG

//配置SPI1
#define DTOF_SPI1_NSS_PIN PLATFORM_SPI1_NSS_PIN
#define DTOF_SPI1_NSS_CFG PLATFORM_SPI1_NSS_CFG
#define DTOF_SPI1_SCK_PIN PLATFORM_SPI1_SCK_PIN
#define DTOF_SPI1_SCK_CFG PLATFORM_SPI1_SCK_CFG
#define DTOF_SPI1_MISO_PIN PLATFORM_SPI1_MISO_PIN
#define DTOF_SPI1_MISO_CFG PLATFORM_SPI1_MISO_CFG
#define DTOF_SPI1_MOSI_PIN PLATFORM_SPI1_MOSI_PIN
#define DTOF_SPI1_MOSI_CFG PLATFORM_SPI1_MOSI_CFG

//配置串口1
#define DTOF_UART1_TX_PIN PLATFORM_UART1_TX_PIN
#define DTOF_UART1_TX_CFG PLATFORM_UART1_TX_CFG
#define DTOF_UART1_RX_PIN PLATFORM_UART1_RX_PIN
#define DTOF_UART1_RX_CFG PLATFORM_UART1_RX_CFG

//配置串口2
#define DTOF_UART2_TX_PIN PLATFORM_UART2_TX_PIN
#define DTOF_UART2_TX_CFG PLATFORM_UART2_TX_CFG
#define DTOF_UART2_RX_PIN PLATFORM_UART2_RX_PIN
#define DTOF_UART2_RX_CFG PLATFORM_UART2_RX_CFG

//配置IIC1
#define DTOF_I2C1_SCL_PIN PLATFORM_I2C1_SCL_PIN
#define DTOF_I2C1_SCL_CFG PLATFORM_I2C1_SCL_CFG
#define DTOF_I2C1_SDA_PIN PLATFORM_I2C1_SDA_PIN
#define DTOF_I2C1_SDA_CFG PLATFORM_I2C1_SDA_CFG

//配置DTOF_RST
#define DTOF_RST_PIN PLATFORM_RST_PIN
#define DTOF_RST_CFG PLATFORM_RST_CFG

//配置DTOF_COMMTYPE_PIN
#define DTOF_COMMTYPE_PIN PLATFORM_COMMTYPE_PIN
#define DTOF_COMMTYPE_CFG PLATFORM_COMMTYPE_CFG

//配置DTOF_INTERRUPT_PIN
#define DTOF_INTERRUPT_PIN PLATFORM_INTERRUPT_PIN
#define DTOF_INTERRUPT_CFG PLATFORM_INTERRUPT_CFG

#define DTOF_SWIT_PIN 0xFFFFFFFF
#define MOS_TASK_MAX_PRIORITY 255
#define DTOF_INT_GERATEST_PRIORITY PLATFORM_INT_GERATEST_PRIORITY

#define DTOF_INT_PRIORITY_GERATEST 0
#define DTOF_INT_PRIORITY_CRITICAL 40
#define DTOF_INT_PRIORITY_HIGHEST 80
#define DTOF_INT_PRIORITY_NORMAL 120
#define DTOF_INT_PRIORITY_BELOW_NORMAL 160
#define DTOF_INT_PRIORITY_BELOW_LOWEST 200
#define DTOF_INT_PRIORITY_BELOW_IDLE 255

#define DTOF_SPI_CPHA     (1<<0)
#define DTOF_SPI_CPOL     (1<<1)
#define DTOF_SPI_LSB      (0<<2)
#define DTOF_SPI_MSB      (1<<2)
#define DTOF_SPI_MASTER   (0<<3)
#define DTOF_SPI_SLAVE    (1<<3)
#define DTOF_SPI_CS_HIGH  (1<<4)
#define DTOF_SPI_NO_CS    (1<<5)
#define DTOF_SPI_MODE_MASK    (DTOF_SPI_CPHA | DTOF_SPI_CPOL | DTOF_SPI_MSB | DTOF_SPI_SLAVE | DTOF_SPI_CS_HIGH | DTOF_SPI_NO_CS)

#define DTOF_SPI_MODE_0       (0 | 0)                        /* CPOL = 0, CPHA = 0 */
#define DTOF_SPI_MODE_1       (0 | DTOF_SPI_CPHA)              /* CPOL = 0, CPHA = 1 */
#define DTOF_SPI_MODE_2       (DTOF_SPI_CPOL | 0)              /* CPOL = 1, CPHA = 0 */
#define DTOF_SPI_MODE_3       (DTOF_SPI_CPOL | DTOF_SPI_CPHA)    /* CPOL = 1, CPHA = 1 */
typedef enum { MOS_GPIO_PIN_RESET = 0U, MOS_GPIO_PIN_SET } MOSGPIO_PinState;

#define CHECK_PERMISSION(iType, iFlag)                               \
    do {                                                             \
        if (MOS_RDONLY == iType) {                                   \
            if (!(iFlag & MOS_READ_FLAG)) return DTOF_RET_ERROR;  \
        } else {                                                     \
            if (!(iFlag & MOS_WRITE_FLAG)) return DTOF_RET_ERROR; \
        }                                                            \
    } while (0)

#ifdef __cplusplus
extern "C" {
#endif

typedef struct mos_i2c_configuration
{
    uint8_t type; //新增type用于区分硬件与模拟i2c
    uint8_t mode;
    uint16_t reserved;
}mos_i2c_configuration_t;

typedef struct mos_i2c_info {
    DTOF_I2C_Handle_t hi2c;
    uint32_t iic_id;
    mos_i2c_configuration_t conf;
    uint32_t flags;
} mos_i2c_info_t;

typedef struct mos_spi_configuration
{
    uint8_t mode;
    uint8_t data_width;
    uint16_t reserved;
    uint32_t max_hz;
}mos_spi_configuration_t;

typedef struct mos_spi_info {
    uint32_t spi_id;
    DTOF_SPI_Handle_t hspi;
    struct mos_spi_configuration conf;
    uint32_t nss_pin;
    uint32_t flags;
} mos_spi_info_t;

typedef struct mos_flash_info {
    uint32_t flags;
} mos_flash_info_t;

typedef struct mos_adc_info {
    DTOF_ADC_Handle_t hadc;
    uint32_t flags;
} mos_adc_info_t;

typedef struct mos_dac_info {
    uint32_t flags;
} mos_dac_info_t;


#define RECEIVE_BUF_LEN 500
typedef struct mos_uart_info {
    DTOF_UART_Handle_t huart;
#ifdef ENABLE_UART_DMA
    DTOF_UART_DMA_Handle_t hdma_usart_tx;
    DTOF_UART_DMA_Handle_t hdma_usart_rx;
#endif
    uint32_t uart_id;
    uint8_t uartrxbuff[10];
    uint8_t rxbuff[RECEIVE_BUF_LEN];
    volatile uint16_t uart_in_cnt;
    volatile uint16_t uart_out_cnt;
    // add the debug info
    uint32_t uart_total_in_cnt;
    uint32_t uart_total_out_cnt;
    uint32_t flags;
} mos_uart_info_t;

typedef struct mos_gpio_info {
    uint32_t gpio;
    uint32_t flags;
} mos_gpio_info_t;

typedef enum
{
    DTOF_I2C_TPYE_HARDWARE=0,
	DTOF_I2C_TPYE_SOFTWARE=1,
}DTOF_I2C_TYPE;

typedef enum
{
	DTOF_I2C_MODE_STANDARD=0,
    DTOF_I2C_MODE_FAST=1,
	DTOF_I2C_MODE_FASTPLUS,
}DTOF_I2C_MODE;
#define DTOF_MUTEX_NORMAL 0
#define DTOF_MUTEX_DEFAULT DTOF_MUTEX_NORMAL
#ifndef __linux__
struct pthread_mutexattr_s {
    uint8_t pshared;
};
typedef struct pthread_mutexattr_s pthread_mutexattr_t;

struct pthread_mutex_s {
    int pid;
    uint8_t type;
    int nlocks;
};
typedef struct pthread_mutex_s pthread_mutex_t;
#endif

#define DTOF_MUTEX_INITIALIZER \
    { .pid = 0, .type = DTOF_MUTEX_DEFAULT, .nlocks = 0 }

#ifndef CLOCK_REALTIME
#define CLOCK_REALTIME 0
#endif

#ifndef CLOCK_MONOTONIC
#define CLOCK_MONOTONIC 1
#endif

#define NSEC_PER_SEC 1000000000
#define USEC_PER_SEC 1000000
#define MSEC_PER_SEC 1000
#define DSEC_PER_SEC 10
#define NSEC_PER_DSEC 100000000
#define USEC_PER_DSEC 100000
#define MSEC_PER_DSEC 100
#define NSEC_PER_MSEC 1000000
#define USEC_PER_MSEC 1000
#define NSEC_PER_USEC 1000
#define DEFAULT_TV_SEC 1661438867
#define DEFAULT_TV_NSEC 368202701

#define DTOF_TIMER_FLAG_DEACTIVATED       0x0
#define DTOF_TIMER_FLAG_ACTIVATED         0x1
#define DTOF_TIMER_FLAG_ONE_SHOT          0x0
#define DTOF_TIMER_FLAG_PERIODIC          0x2


#define DTOF_TIMER_SKIP_LIST_LEVEL  1
#define DTOF_TICK_MAX                     4294967295u
#define DTOF_TIMER_SKIP_LIST_MASK         0x3

struct dtof_timer
{
//     struct list_head        row[DTOF_TIMER_SKIP_LIST_LEVEL];
    void (*timeout_func)(void *parameter);
    void            *parameter;
    dtof_tick_t        init_tick;
    dtof_tick_t        timeout_tick;
    uint8_t flag;
};
typedef struct dtof_timer *dtof_timer_t;


struct dtof_rw_register {
    uint8_t reg_addr;
    uint16_t reg_val;
};

struct dtof_readram {
    uint16_t len;
    uint16_t offset;
    uint16_t* buf;
};

struct dtof_burst {
    uint8_t reg_addr;
    uint8_t* start;
    uint16_t len;
};

#define DTOF_REGISTERS_BUF_SIZE	64
#define DTOF_RW_REG_BATCH_NUM_MAX	16


typedef struct dtof_registers_s {
	uint8_t is_write;
    uint8_t start_addr;
    uint16_t len;
	uint32_t delay_us;
    uint16_t buf[DTOF_REGISTERS_BUF_SIZE];
}dtof_registers_t;


typedef struct dtof_rw_reg_batch_s
{
	uint8_t len;
	dtof_registers_t reg_data[DTOF_RW_REG_BATCH_NUM_MAX];
}dtof_rw_reg_batch_t;

typedef struct dtof_state_s
{
    uint32_t reset_state;
    uint32_t reset_count;
}dtof_state_t;

typedef enum
{
    DTOF_READ_REG = 1, //struct dtof_rw_register
    DTOF_WRITE_REG,    //struct dtof_rw_register
    DTOF_READ_FRAME,   //struct dtof_readram
    DTOF_RESET,
    DTOF_WRITE_BURST,  //struct dtof_burst
    DTOF_READ_BURST,   //struct dtof_burst
    DTOF_RW_REG_BATCH,  //struct dtof_rw_reg_batch_s
    DTOF_GET_STATE,     //struct dtof_state_s
    DTOF_RGLTR_CTRL,
    DTOF_DEBUG_0,
    DTOF_RESET_HIGH,    //only reset，not config pll
    DTOF_RETRIEVE_DEV_ADDRESS =64, //get the current bus communication device address
    DTOF_OPTION_DEV_ADDRESS, //set the current bus communication device address
    DTOF_RETRIEVE_DEV_SPEED,
    DTOF_OPTION_DEV_SPEED
}dtof_cmd_e;


#define DTOF_WRITE_REG _IOR('A', 1, struct dtof_rw_register)
#define DTOF_READ_REG _IOR('A', 2, struct dtof_rw_register)
#define DTOF_READ_FRAME _IOR('A', 3, struct dtof_readram)
#define DTOF_DEBUG_0 _IO('A', 20)


#ifdef __cplusplus
}
#endif

#endif  // DRIVERS_BASE_INCLUDE_MOS_DEF_H_
