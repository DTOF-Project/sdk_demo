#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "inc/dtof_base_type.h"
#include "inc/dtof_driver.h"
#include "inc/dtof_api.h"
#include "inc/dtof_log.h"
#include "inc/app_cmd.h"

#define ECO_TEST_MODE

#define UART_BUF_SIZE 128

static char uart_buf[UART_BUF_SIZE];
static int buf_pos = 0;

// ========== 命令处理函数 ==========
void app_cmd_start_distance_measure(const char *cmd) {
    // DTOF_CHECK_WARN(dtof_sensor_init(), "dtof sensor init failed\n");
    // dtof_start_distance_measure();
    printf("[CMD] s -> start distance measure\n");
}

void app_cmd_stop_distance_measure(const char *cmd) {
    // DTOF_CHECK_WARN(dtof_sensor_init(), "dtof sensor init failed\n");
    // dtof_start_distance_measure();
    printf("[CMD] s -> stop distance measure\n");
}


void app_cmd_start_distance_measure_debug_mode(const char *cmd) {
    // DTOF_CHECK_WARN(dtof_sensor_init(), "dtof sensor init failed\n");
    // dtof_start_distance_measure();
    // if (dev->chip_type == DTOF_CHIP_TYPE_A05) {
    //     DTOF_CHECK_WARN(
    //         dtof_io_interaction(DTOF_CMD_WRITE_REG_ADDR, SPECIAL_BYPASS_VALUE),
    //         "enable debug mode failed\n"
    //     );
    // }
    // is_init = DTOF_TRUE;
    // debug_flag = DTOF_TRUE;
    printf("[CMD] d -> init debug mode\n");
}

void app_cmd_start_distance_measure_test_mode(const char *cmd) {
    // DTOF_CHECK_WARN(dtof_sensor_init(), "dtof sensor init failed\n");
    // dtof_start_distance_measure();
    // frame_cnt_flag = DTOF_TRUE;
    // is_init = DTOF_TRUE;
    // debug_flag = DTOF_TRUE;
    printf("[CMD] e -> enable frame count + debug\n");
}

void app_cmd_reg_burst_read(const char *cmd) {
    // DTOF_CHECK_RET(dtof_set_mcu_status_ram(DTOF_MCU_STATE_SLEEP_DIRECT),
    //                "set mcu sleep failed\n");

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

    // DTOF_CHECK_RET(dtof_set_mcu_status_ram(DTOF_MCU_STATE_WAKEUP),
    //                "wakeup mcu failed\n");
    printf("[CMD] rb -> burst read\n");
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
        dtof_write_reg_running(reg_addr, reg_value); // 示例：写入值为索引 i，你可根据实际需求改成 uart_buf 中解析的值
        printf("reg write 0x%x: 0x%04x\n", reg_addr, reg_value);
    }
}

#ifdef ECO_TEST_MODE
void app_cmd_test_ram_code_burn(const char *cmd) {
    int ret;
    uint16_t ram_code[] =  {0xe977, 0x3707, 0x0093, 0x9387, 0xd71e, 0x2314, 0xf70c, 0x8280, 0x0000, 0x0000, 0x7f00, 0x0c00, };;
    ret = dtof_ram_code_burn(ram_code, sizeof(ram_code)/sizeof(ram_code[0]), DTOF_SWB_TYPE_FROM_RAM_FIXADDR, 0x0);
    if (ret != DTOF_RET_SUCCESS) {
        printf("ram code burn failed, ret = %d\n", ret);
    } else {
        printf("ram code burn success\n");
    }

    printf("[CMD] test0 -> test ram code burn\n");
}
#endif

cmd_entry_t cmd_table[] = {
    { "s",   0, app_cmd_start_distance_measure },
    { "t",   0, app_cmd_stop_distance_measure },
    { "d",   0, app_cmd_start_distance_measure_debug_mode },
    { "e",   0, app_cmd_start_distance_measure_test_mode },
    { "ri,", 1, app_cmd_reg_read_running },
    { "wi,", 1, app_cmd_reg_write_running },
    { "rb,", 1, app_cmd_reg_burst_read },

#ifdef ECO_TEST_MODE
    // test cmd
    { "test0",   0, app_cmd_test_ram_code_burn },
#endif
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
    printf("Unknown UART command: %s\n", uart_buf);
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
