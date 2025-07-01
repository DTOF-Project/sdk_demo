#ifndef DEVICE_H_
#define DEVICE_H_

typedef struct {
    DTOF_RET (*init)(int device_id);
    DTOF_RET (*deinit)(int device_id);
    DTOF_RET (*read)(int device_id, void *buf, int nbyte);
    DTOF_RET (*write)(int device_id, void *buf, int nbyte);
    DTOF_RET (*write_word)(int device_id, uint8_t reg, const uint16_t val);
    DTOF_RET (*write_block)(int device_id, uint8_t reg, uint8_t *input_buf, uint16_t input_len);
    DTOF_RET (*read_word)(int device_id, uint8_t reg, uint16_t *buf);
    DTOF_RET (*read_block)(int device_id, uint8_t reg, uint8_t *output_buf, uint16_t read_len);
    DTOF_RET (*configure)(int device_id, void *cfg);
    DTOF_RET (*get_attribute)(int device_id, int attribute, void *attr, uint32_t *attrlen);
    DTOF_RET (*set_attribute)(int device_id, int attribute, void *attr, uint32_t attrLen);
    DTOF_RET (*ioctl)(int device_id, int cmd, void *arg);
} device_driver_ops_t;

typedef DTOF_RET (*gpio_isr_fn)(int irq, void *context, void *priv);

typedef struct{
    DTOF_RET (*init_gpio_fn)(uint32_t gpio, uint32_t cfgset);
    dtof_bool_t (*gpio_is_init_fn)(uint32_t gpio);
    DTOF_RET (*deinit_gpio_fn)(uint32_t gpio);
    DTOF_RET (*read_gpio_fn)(uint32_t gpio, uint32_t *value);
    DTOF_RET (*write_gpio_fn)(uint32_t gpio, uint32_t value);
    DTOF_RET (*irq_attach_fn)(uint32_t gpio, int priority, gpio_isr_fn isr, void *isr_data);
    DTOF_RET (*irq_enable_fn)(uint32_t gpio, dtof_bool_t enabled);
    DTOF_RET (*irq_detach_fn)(uint32_t gpio);
    DTOF_RET (*irq_clear_fn)(uint32_t gpio);
    DTOF_RET (*irq_set_debounce_fn)(uint32_t gpio, uint16_t delay);
    DTOF_RET(*swit_attach_fn)(uint32_t gpio, int priority, gpio_isr_fn isr, void *isr_data, int *irq);
    DTOF_RET (*generate_swit_fn)(uint32_t gpio, int irq);
    DTOF_RET (*rcc_fn)(uint32_t gpio, dtof_bool_t eb);
} device_driver_gpio_ops_t;

// spi/iic
DTOF_RET device_init(dtof_uint32_t device_id);
DTOF_RET device_deinit(dtof_uint32_t device_id);
DTOF_RET device_read_block(dtof_uint32_t device_id, uint8_t reg, uint8_t *output_buf, uint16_t read_len);
DTOF_RET device_write_block(dtof_uint32_t device_id, uint8_t reg, uint8_t *input_buf, uint16_t input_len);

// uart
DTOF_RET device_uart_init(dtof_int32_t device_id);
DTOF_RET device_uart_deinit(dtof_int32_t device_id);
DTOF_RET device_uart_read(dtof_int32_t device_id, void *buf, dtof_int32_t nbyte);
DTOF_RET device_uart_write(dtof_int32_t device_id, void *buf, dtof_int32_t nbyte);

// gpio
DTOF_RET device_gpio_init(dtof_uint32_t gpio, dtof_uint32_t cfgset);
DTOF_RET device_gpio_deinit(dtof_uint32_t gpio);
DTOF_RET device_gpio_read(dtof_uint32_t gpio, dtof_uint32_t *value);
DTOF_RET device_gpio_write(dtof_uint32_t gpio, dtof_uint32_t value);
DTOF_RET device_gpio_irq_attach(dtof_uint32_t gpio, int priority, gpio_isr_fn isr, void *isr_data);


#endif