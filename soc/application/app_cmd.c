#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "inc/dtof_base_type.h"
#include "inc/dtof_driver.h"
#include "inc/dtof_api.h"
#include "inc/dtof_log.h"
#include "inc/dtof_endian.h"
#include "inc/dtof_global_config.h"
#include "inc/dtof_calibration_ft.h"

#include "application/inc/soc_version.h"
#include "application/inc/app_cmd.h"
#include "application/inc/app_distance.h"

// TODO: 不要放在stm32的文件夹下
#include "dev/dtof_hal.h"
#include "customer/dtof_customer.h"

#define ECO_TEST_MODE

#define UART_BUF_SIZE 128

static char uart_buf[UART_BUF_SIZE];
static int buf_pos = 0;

#define RUNNING_RATE_30HZ  30
#define RUNNING_RATE_120HZ 120
static int g_running_rate = RUNNING_RATE_30HZ;

// ========== 命令处理函数 ==========
void app_cmd_start_distance_measure(const char *cmd) {
    app_set_distance_mode(DISTANCE_NORMAL_MODE);
    DTOF_CHECK_WARN(dtof_sensor_init(), "dtof sensor init failed\n");
    DTOF_CHECK_WARN(dtof_start_distance_measure(), "dtof start distance mode failed\n");
}

void app_cmd_stop_distance_measure(const char *cmd) {
    // app_set_distance_mode(DISTANCE_UNKNOWN_MODE);
    DTOF_CHECK_WARN(dtof_stop_distance_measure(), "dtof stop distance mode failed\n");
    dtof_sleep_ms(33); //TODO: 待优化, 要等可能存在的上一帧跑完
    dtof_set_interrupt_flag(DTOF_FALSE);
    app_set_distance_mode(DISTANCE_UNKNOWN_MODE);
}

void app_cmd_start_distance_measure_debug_mode(const char *cmd) {
    app_set_distance_mode(DISTANCE_DEBUG_MODE);
    DTOF_CHECK_WARN(dtof_sensor_init(), "dtof sensor init failed\n");
    DTOF_CHECK_WARN(dtof_start_distance_measure(), "dtof start distance mode failed\n");
    DTOF_CHECK_WARN(dtof_enable_distance_debug_mode(), "enable debug mode failed\n");
}

void app_cmd_start_distance_measure_test_mode(const char *cmd) {
    app_set_distance_mode(DISTANCE_TEST_MODE);
    DTOF_CHECK_WARN(dtof_sensor_init(), "dtof sensor init failed\n");
    DTOF_CHECK_WARN(dtof_start_distance_measure(), "dtof start distance mode failed\n");
    dtof_sleep_ms(1);
    DTOF_CHECK_WARN(dtof_enable_distance_debug_mode(), "enable debug mode failed\n");
}

void app_cmd_reg_burst_read(const char *cmd) {
    DTOF_CHECK_WARN(dtof_set_mcu_status_ram(DTOF_MCU_STATE_SLEEP_DIRECT), "set mcu sleep failed\n");

    int reg_addr, reg_num;
    dtof_uint16_t reg_max[255];

    if (sscanf(cmd, "rb,%d,%d", &reg_addr, &reg_num) == 2) {
        dtof_reg_burst_read(reg_addr, reg_max, reg_num);
        dtof_printf("burst reg read 0x%04x:\n", reg_addr);
        for (int i = 0; i < reg_num; i++) {
            dtof_printf("%d, ", reg_max[i]);
        }
        dtof_printf("\n");
    }

    DTOF_CHECK_WARN(dtof_set_mcu_status_ram(DTOF_MCU_STATE_WAKEUP), "wakeup mcu failed\n");
}

void app_cmd_reg_read_running(const char *cmd) {
    int reg_addr = atoi(&cmd[3]);
    uint16_t reg_value;
    DTOF_CHECK_RET_VOID(dtof_read_reg_running(reg_addr, &reg_value), "read reg running failed\n");
    dtof_printf("reg read 0x%x: 0x%4x\n", reg_addr, reg_value);
}

void app_cmd_reg_write_running(const char *cmd) {
    int reg_addr, reg_value;
    if (sscanf(uart_buf, "wi,%d,%d", &reg_addr, &reg_value) == 2)
    {
        dtof_write_reg_running(reg_addr, reg_value);
        dtof_printf("reg write 0x%x: 0x%04x\n", reg_addr, reg_value);
    }
}

void app_cmd_get_version(const char *cmd) {
    DTOF_LOG("soc version: %s\n", SOC_VERSION_STRING);
    DTOF_LOG("sdk version: %s\n", dtof_get_sdk_version());
    DTOF_LOG("ram version: %d\n", DTOF_SWAP16(dtof_get_chip_version()));
    DTOF_LOG("chip id: 0x%04x\n", dtof_get_chip_config()->chip_id);
    DTOF_LOG("chip uuid: ");
    for (int i = 0; i < DTOF_UUID_LENGTH; i++)
    {
        dtof_printf("%d, ", dtof_get_chip_config()->chip_uuid[i]);
    }
}

void app_cmd_print_chip_info(const char *cmd) {
    dtof_printf("chip uuid: ");
    for (int i = 0; i < DTOF_UUID_LENGTH; i++)
    {
        dtof_printf("%d, ", dtof_get_chip_config()->chip_uuid[i]);
    }
    dtof_printf("\n");
    dtof_printf("otp list:\n");
    dtof_uint8_t otp_data[128];
    dtof_set_mcu_status_ram(DTOF_MCU_STATE_SLEEP_DIRECT);
    DTOF_CHECK_WARN(dtof_read_otp(0, otp_data, 128), "read otp failed\n");
    dtof_set_mcu_status_ram(DTOF_MCU_STATE_WAKEUP);
    for (int i = 0; i < 128; i++)
    {
        dtof_printf("%d, ", otp_data[i]);
    }
    dtof_printf("\n");

    dtof_ft_cali_param_t ft_data_read;
    dtof_bool_t is_legal_data = DTOF_FALSE;
    dtof_get_ft_data_from_flash((dtof_uint16_t*)&ft_data_read, sizeof(dtof_ft_cali_param_t)/sizeof(dtof_uint16_t), &is_legal_data);
    if (is_legal_data == DTOF_FALSE)
    {
        dtof_printf("ft data is illegal, all 0xFF\n");
    }
    else
    {
        dtof_uint16_t ft_cali_type = ~(ft_data_read.ft_calibration_type);
        if(DTOF_BIT_GET(ft_cali_type, DTOF_FT_CALIBRATE_BINOFFSET))
        {
            dtof_printf("bin_offset = %u\n", ft_data_read.dtof_ft_data.bin_offset);
        }
        if(DTOF_BIT_GET(ft_cali_type, DTOF_FT_CALIBRATE_REFSPAD))
        {
            dtof_printf("ref_spad = %u\n", ft_data_read.dtof_ft_data.ref_spad);
        }
        if(DTOF_BIT_GET(ft_cali_type, DTOF_FT_CALIBRATE_CG))
        {
            dtof_printf("cg_reg: ");
            dtof_uint16_t cg_reg;
            for (int i = 0; i < (DTOF_AC_NUM + 1); i++)
            {
                cg_reg = ft_data_read.dtof_ft_data.cg_data[i * 2 + 1] * 256 + ft_data_read.dtof_ft_data.cg_data[i * 2];
                dtof_printf("%u, ", cg_reg);
            }
            dtof_printf("\n");
        }
        if(DTOF_BIT_GET(ft_cali_type, DTOF_FT_CALIBRATE_B))
        {
            dtof_printf("distance_k=%d, distance_b=%d\n", ft_data_read.dtof_ft_data.distance_k, ft_data_read.dtof_ft_data.distance_b);
        }
    }

    dtof_printf("running rate: %d Hz\n", g_running_rate);
}

void app_cmd_clear_cal_info(const char *cmd) {
    // uint16_t a = 0xfffb;
    // uint16_t b = 0xfff3;
    // uint16_t a_r;
    // uint16_t b_r;
    stm32_flash_write_init(DTOF_FT_DATA_FLASH_PAGE, DTOF_FT_DATA_FLASH_PAGE_NUM);
    // stm32_flash_write_u64(DTOF_FT_DATA_FLASH_PAGE_START_ADDR, (uint64_t *)&a, 1);
    // stm32_flash_read_u64(DTOF_FT_DATA_FLASH_PAGE_START_ADDR, (uint64_t *)&a_r, 1);
    // printf("a = 0x%04x, a_r = 0x%04x\n", a, a_r);
    // stm32_flash_write_u64(DTOF_FT_DATA_FLASH_PAGE_START_ADDR, (uint64_t *)&b, 1);
    // stm32_flash_read_u64(DTOF_FT_DATA_FLASH_PAGE_START_ADDR, (uint64_t *)&b_r, 1);
    // printf("a = 0x%04x, a_r = 0x%04x\n", b, b_r);
}

static dtof_uint16_t is_to_sky_flag = 1;
void app_cmd_do_ft_calibration(const char *cmd) {
    // stm32_flash_write_init(DTOF_FT_DATA_FLASH_PAGE, DTOF_FT_DATA_FLASH_PAGE_NUM);
    // DTOF_CHECK_WARN(dtof_sensor_init(), "dtof sensor init failed\n");
    dtof_uint16_t ft_cali_type;
    dtof_uint16_t ft_cali_param; // 可以是otp_ref_spad_mask, 也可以是distance, 目前这种设计下, 不能同时做refspad和b校准

    if (sscanf(cmd, "ft,%hu,%hu", &ft_cali_type, &ft_cali_param) == 2)
    {
        // 设置校准类型
        dtof_set_ft_calibration_type(ft_cali_type);
        dtof_printf("start ft calibration, type=0x%04x\n", ft_cali_type);
        DTOF_RET ret = dtof_do_ft_calibration(ft_cali_param, ft_cali_param, ft_cali_param);
        if (ret == DTOF_RET_SUCCESS) {
            dtof_ft_cali_param_t ft_cali_param;
            dtof_bool_t is_legal_data = DTOF_FALSE;
            dtof_get_ft_data_from_flash((dtof_uint16_t*)&ft_cali_param, sizeof(dtof_ft_cali_param_t)/sizeof(dtof_uint16_t), &is_legal_data);
            dtof_printf("FT success:\n");
            if (is_legal_data != DTOF_TRUE)
            {
                dtof_printf("ft data is illegal\n");
                return;
            }
            if(DTOF_BIT_GET(ft_cali_type, DTOF_FT_CALIBRATE_BINOFFSET))
            {
                dtof_printf("bin_offset = %u\n", ft_cali_param.dtof_ft_data.bin_offset);
            }
            if(DTOF_BIT_GET(ft_cali_type, DTOF_FT_CALIBRATE_REFSPAD))
            {
                dtof_printf("ref_spad = %u\n", ft_cali_param.dtof_ft_data.ref_spad);
            }
            if(DTOF_BIT_GET(ft_cali_type, DTOF_FT_CALIBRATE_CG))
            {
                dtof_printf("cg_reg: ");
                dtof_uint16_t cg_reg;
                for (int i = 0; i < (DTOF_AC_NUM + 1); i++)
                {
                    cg_reg = ft_cali_param.dtof_ft_data.cg_data[i * 2 + 1] * 256 + ft_cali_param.dtof_ft_data.cg_data[i * 2];
                    dtof_printf("%u, ", cg_reg);
                }
                dtof_printf("\n");
            }
            if(DTOF_BIT_GET(ft_cali_type, DTOF_FT_CALIBRATE_B))
            {
                dtof_printf("distance_k=%d, distance_b=%d\n", ft_cali_param.dtof_ft_data.distance_k, ft_cali_param.dtof_ft_data.distance_b);
            }
        }
        else{
            dtof_printf("ft calibration failed\n");
        }
    }
}

void app_cmd_set_refspad(const char *cmd) {
    dtof_uint16_t ref_spad;
    if (sscanf(cmd, "refspad,%hu", &ref_spad) == 1)
    {
        dtof_printf("set refspad = %u\n", ref_spad);
        #define DTOF_FT_DATA_START 0x2000
        #define DTOF_FT_DATA_B_OFFSET 4
        DTOF_CHECK_RET_VOID(dtof_set_mcu_status_ram(DTOF_MCU_STATE_SLEEP_DIRECT), "MCU sleep failed");

        dtof_uint16_t ram_data[2];
        dtof_uint16_t dtof_ft_data_start = DTOF_FT_DATA_START + dtof_get_chip_config()->version_lenth - DTOF_FT_DATA_B_OFFSET;

        DTOF_CHECK_RET_VOID(dtof_reg_burst_write(DTOF_WRITE_RAM_START_REG_ADDR, &dtof_ft_data_start, 1), "write ram start addr failed\n");
        DTOF_CHECK_RET_VOID(dtof_reg_burst_read(DTOF_READ_RAM_START_REG_ADDR, ram_data, sizeof(ram_data)/sizeof(ram_data[0])), "read ft data failed\n");

        ram_data[0] = ref_spad;
        DTOF_CHECK_RET_VOID(dtof_reg_burst_write(DTOF_WRITE_RAM_START_REG_ADDR, &dtof_ft_data_start, 1), "write ram start addr failed\n");
        DTOF_CHECK_RET_VOID(dtof_reg_burst_write(DTOF_READ_RAM_START_REG_ADDR, ram_data, sizeof(ram_data)/sizeof(ram_data[0])), "read ft data failed\n");

        DTOF_CHECK_RET_VOID(dtof_set_mcu_status_ram(DTOF_MCU_STATE_WAKEUP), "MCU sleep failed");

        dtof_reload_ft_data();
    }
}

void app_cmd_set_frame_rate(const char *cmd) {
    dtof_uint16_t frame_rate;
    if (sscanf(cmd, "rate,%hu", &frame_rate) == 1)
    {
        dtof_printf("set frame rate = %u Hz\n", frame_rate);
        if (frame_rate == RUNNING_RATE_30HZ) {
            dtof_switch_frame_rate(DTOF_30HZ_FRAME_CNT);
            g_running_rate = RUNNING_RATE_30HZ;
        } else if (frame_rate == RUNNING_RATE_120HZ) {
            dtof_switch_frame_rate(DTOF_120HZ_FRAME_CNT);
            g_running_rate = RUNNING_RATE_120HZ;
        }
        else {
            dtof_printf("unsupported frame rate, only support 30Hz and 120Hz\n");
        }
    }
}

cmd_entry_t cmd_table[] = {
    { "s",   0, app_cmd_start_distance_measure },
    { "t",   0, app_cmd_stop_distance_measure },
    { "d",   0, app_cmd_start_distance_measure_debug_mode },
    { "e",   0, app_cmd_start_distance_measure_test_mode },
    { "clear", 0, app_cmd_clear_cal_info },
    { "v",   0, app_cmd_get_version },
    { "p",   0, app_cmd_print_chip_info },
    { "ri,", 1, app_cmd_reg_read_running },
    { "wi,", 1, app_cmd_reg_write_running },
    { "rb,", 1, app_cmd_reg_burst_read },
    { "ft,", 1, app_cmd_do_ft_calibration },
    {"refspad,", 1, app_cmd_set_refspad },
    {"rate,", 1, app_cmd_set_frame_rate },
};

// ========== 命令解析器 ==========
static void handle_uart_cmd(const char *uart_buf) {
    size_t table_size = sizeof(cmd_table) / sizeof(cmd_table[0]);
    for (size_t i = 0; i < table_size; i++) {
        if (cmd_table[i].is_prefix) {
            if (strncmp(uart_buf, cmd_table[i].name, strlen(cmd_table[i].name)) == 0) {
                cmd_table[i].handler(uart_buf);
                return;
            }
        } else {
            if (strcmp(uart_buf, cmd_table[i].name) == 0) {
                cmd_table[i].handler(uart_buf);
                return;
            }
        }
    }
    dtof_printf("unknown command: %s\n", uart_buf);
}

static int is_end_of_command(char byte) {
    return (byte == '\n' || byte == '\r');
}

static void reset_uart_buffer(void) {
    buf_pos = 0;
    memset(uart_buf, 0, sizeof(uart_buf));
}

void parse_cmd_process(char byte)
{
    if (is_end_of_command(byte)) {
        uart_buf[buf_pos] = '\0';    // 结束符
        // handle_uart_cmd(uart_buf);   // 处理命令
        // 去掉末尾的 \r 或 \n, 防止空命令
        while (buf_pos > 0 && (uart_buf[buf_pos - 1] == '\r' || uart_buf[buf_pos - 1] == '\n'))
        {
            uart_buf[--buf_pos] = '\0';
        }

        if (buf_pos > 0) {
            handle_uart_cmd(uart_buf);
        }

        reset_uart_buffer();
        return;
    }

    if (buf_pos < UART_BUF_SIZE - 1) {
        uart_buf[buf_pos++] = byte;
    } else {
        // 缓冲区溢出
        reset_uart_buffer();
    }
    return;
}
