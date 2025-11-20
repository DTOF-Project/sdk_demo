#define DTOF_FIFO_START_ADDR   0x0400
#define DTOF_SINGLE_FIFO_LEN   0x0019
#define DTOF_MULTIPLE_FIFO_LEN 0x001B
#define DTOF_FIFO_LEN (DTOF_SINGLE_FIFO_LEN + DTOF_MULTIPLE_FIFO_LEN)
#define DTOF_SINGLE_MAIN_HISTGRAM_OFFSET   0
#define DTOF_SINGLE_MAIN_HISTGRAM_LEN      512 // 64 * 8
#define DTOF_SINGLE_REF_HISTGRAM_OFFSET    512
#define DTOF_SINGLE_REF_HISTGRAM_LEN       64

DTOF_RET dtof_histgram_io_read(dtof_uint16_t address, dtof_uint16_t * value_p, dtof_uint16_t len);
DTOF_RET dtof_dsp_fifo_read(dtof_uint16_t addr_offset, dtof_uint16_t * value_p, dtof_uint16_t len);
DTOF_RET dtof_read_reg_running(dtof_uint16_t reg_addr, dtof_uint16_t *reg_data);
DTOF_RET dtof_write_reg_running(dtof_uint16_t reg_addr, dtof_uint16_t reg_data);
DTOF_RET dtof_read_otp(dtof_uint8_t device_id, dtof_uint8_t offset, dtof_uint8_t *buf, dtof_uint16_t len);
