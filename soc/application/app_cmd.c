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

// ========== 命令处理函数 ==========
void app_cmd_start_distance_measure(const char *cmd) {
    app_set_distance_mode(DISTANCE_NORMAL_MODE);
    DTOF_CHECK_WARN(dtof_sensor_init(), "dtof sensor init failed\n");
    DTOF_CHECK_WARN(dtof_start_distance_measure(), "dtof start distance mode failed\n");
}

void app_cmd_stop_distance_measure(const char *cmd) {
    // app_set_distance_mode(DISTANCE_UNKNOWN_MODE);
    DTOF_CHECK_WARN(dtof_stop_distance_measure(), "dtof stop distance mode failed\n");
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
        printf("burst reg read 0x%04x:\n", reg_addr);
        for (int i = 0; i < reg_num; i++) {
            printf("%d, ", reg_max[i]);
        }
        printf("\n");
    }

    DTOF_CHECK_WARN(dtof_set_mcu_status_ram(DTOF_MCU_STATE_WAKEUP), "wakeup mcu failed\n");
}

void app_cmd_reg_read_running(const char *cmd) {
    int reg_addr = atoi(&cmd[3]);
    uint16_t reg_value;
    DTOF_CHECK_RET_VOID(dtof_read_reg_running(reg_addr, &reg_value), "read reg running failed\n");
    printf("reg read 0x%x: 0x%4x\n", reg_addr, reg_value);
}

void app_cmd_reg_write_running(const char *cmd) {
    int reg_addr, reg_value;
    if (sscanf(uart_buf, "wi,%d,%d", &reg_addr, &reg_value) == 2)
    {
        dtof_write_reg_running(reg_addr, reg_value);
        printf("reg write 0x%x: 0x%04x\n", reg_addr, reg_value);
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
        printf("%d, ", dtof_get_chip_config()->chip_uuid[i]);
    }
}

void app_cmd_print_chip_info(const char *cmd) {
    printf("chip uuid: ");
    for (int i = 0; i < DTOF_UUID_LENGTH; i++)
    {
        printf("%d, ", dtof_get_chip_config()->chip_uuid[i]);
    }
    printf("\n");
    printf("otp list:\n");
    dtof_uint8_t otp_data[128];
    DTOF_CHECK_WARN(dtof_read_otp(0, otp_data, 128), "read otp failed\n");
    for (int i = 0; i < 128; i++)
    {
        printf("%d, ", otp_data[i]);
    }
    printf("\n");

    dtof_ft_data_t ft_data_read;
    dtof_bool_t is_legal_data = DTOF_FALSE;
    dtof_get_ft_data_from_flash((dtof_uint16_t*)&ft_data_read, sizeof(dtof_ft_data_t)/sizeof(dtof_uint16_t), &is_legal_data);
    if (is_legal_data == DTOF_FALSE)
    {
        printf("ft data is illegal, all 0xFF\n");
    }
    else
    {
        #ifdef DTOF_FT_CALIBRATE_BINOFFSET
        printf("bin_offset = %u\n", ft_data_read.bin_offset);
        #endif
        #ifdef DTOF_FT_CALIBRATE_REFSPAD
        printf("ref_spad = %u\n", ft_data_read.ref_spad);
        #endif
        #ifdef DTOF_FT_CALIBRATE_CG
        printf("cg_reg: ");
        dtof_uint16_t cg_reg;
        for (int i = 0; i < (DTOF_AC_NUM + 1); i++)
        {
            cg_reg = ft_data_read.cg_data[i * 2 + 1] * 256 + ft_data_read.cg_data[i * 2];
            printf("%u, ", cg_reg);
        }
        printf("\n");
        #endif
        #ifdef DTOF_FT_CALIBRATE_B
        printf("distance_k=%d, distance_b=%d\n", ft_data_read.distance_k, ft_data_read.distance_b);
        #endif
        }
}

void app_cmd_clear_cal_info(const char *cmd) {
    stm32_flash_write_init(DTOF_FT_DATA_FLASH_PAGE, DTOF_FT_DATA_FLASH_PAGE_NUM);
}

static dtof_uint16_t is_to_sky_flag = 1;
void app_cmd_do_ft_calibration(const char *cmd) {
    stm32_flash_write_init(DTOF_FT_DATA_FLASH_PAGE, DTOF_FT_DATA_FLASH_PAGE_NUM);
    DTOF_CHECK_WARN(dtof_sensor_init(), "dtof sensor init failed\n");
    dtof_uint16_t otp_ref_spad_mask,distance;

    if (sscanf(cmd, "ft,%hu,%hu", &otp_ref_spad_mask, &distance) == 2)
    {
        DTOF_RET ret = dtof_do_ft_calibration(otp_ref_spad_mask, distance, is_to_sky_flag);
        if (ret == DTOF_RET_SUCCESS) {
            dtof_ft_data_t ft_data;
            dtof_bool_t is_legal_data = DTOF_FALSE;
            dtof_get_ft_data_from_flash((dtof_uint16_t*)&ft_data, sizeof(dtof_ft_data_t)/sizeof(dtof_uint16_t), &is_legal_data);
            printf("FT success:\n");
            if (is_legal_data != DTOF_TRUE)
            {
                printf("ft data is illegal\n");
                return;
            }
            #ifdef DTOF_FT_CALIBRATE_BINOFFSET
            printf("bin_offset = %u\n", cal_data.binoffset_cal_data.binoffset);
            #endif
            #ifdef DTOF_FT_CALIBRATE_REFSPAD
            printf("otp_ref_spad_mask = %u, ref_spad = %u\n", cal_data.ref_spad_cal.otp_ref_spad_mask, cal_data.ref_spad_cal.ref_spad);
            #endif
            #ifdef DTOF_FT_CALIBRATE_CG
            printf("cg_reg: ");
            dtof_uint16_t cg_reg;
            for (int i = 0; i < (DTOF_AC_NUM + 1); i++)
            {
                cg_reg = ft_data.cg_data[i * 2 + 1] * 256 + ft_data.cg_data[i * 2];
                printf("%u, ", cg_reg);
            }
            printf("\n");
            #endif
            #ifdef DTOF_FT_CALIBRATE_B
            printf("distance = %u, distance_k=%d, distance_b=%d\n", distance, ft_data.distance_k, ft_data.distance_b);
            #endif

        }
        else{
            printf("ft calibration failed\n");
        }
    }
}

cmd_entry_t cmd_table[] = {
    { "s",   0, app_cmd_start_distance_measure },
    { "t",   0, app_cmd_stop_distance_measure },
    { "d",   0, app_cmd_start_distance_measure_debug_mode },
    { "e",   0, app_cmd_start_distance_measure_test_mode },
    { "ri,", 1, app_cmd_reg_read_running },
    { "wi,", 1, app_cmd_reg_write_running },
    { "rb,", 1, app_cmd_reg_burst_read },
    { "ft,", 1, app_cmd_do_ft_calibration },
    { "v",   0, app_cmd_get_version },
    { "p",   0, app_cmd_print_chip_info },
    { "clear", 0, app_cmd_clear_cal_info },
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
    printf("unknown command: %s\n", uart_buf);
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
