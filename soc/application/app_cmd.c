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
#include "inc/dev/dtof_dev_api.h"

#include "application/inc/soc_version.h"
#include "application/inc/app_cmd.h"
#include "application/inc/app_distance.h"

#include "inc/dev/dtof_hal.h"
#include "customer/dtof_customer.h"

#define ECO_TEST_MODE

#define UART_BUF_SIZE 128

static char uart_buf[UART_BUF_SIZE];
static int buf_pos = 0;

static int is_end_of_command(char byte) {
    return (byte == '\n' || byte == '\r');
}

static void reset_uart_buffer(void) {
    buf_pos = 0;
    memset(uart_buf, 0, sizeof(uart_buf));
}

// ========== 命令处理函数 ==========
void app_cmd_start_distance_measure(const char *cmd) {
    app_set_distance_mode(DISTANCE_NORMAL_MODE);
    DTOF_CHECK_RET_VOID(dtof_sensor_init(), "dtof sensor init failed\n");
    DTOF_CHECK_RET_VOID(dtof_start_distance_measure(), "dtof start distance mode failed\n");
}

void app_cmd_stop_distance_measure(const char *cmd) {
    DTOF_CHECK_RET_VOID(dtof_stop_distance_measure(), "dtof stop distance mode failed\n");
}

void app_cmd_start_distance_measure_debug_mode(const char *cmd) {
    app_set_distance_mode(DISTANCE_DEBUG_MODE);
    DTOF_CHECK_RET_VOID(dtof_sensor_init(), "dtof sensor init failed\n");
    DTOF_CHECK_RET_VOID(dtof_start_distance_measure_one_frame(), "dtof start distance mode failed\n");
}

void app_cmd_start_distance_measure_test_mode(const char *cmd) {
    app_set_distance_mode(DISTANCE_TEST_MODE);
    DTOF_CHECK_RET_VOID(dtof_sensor_init(), "dtof sensor init failed\n");
    DTOF_CHECK_RET_VOID(dtof_start_distance_measure_one_frame(), "dtof start distance mode failed\n");
}

void app_cmd_reg_burst_read(const char *cmd) {
    DTOF_CHECK_WARN(dtof_set_mcu_status_ram(DTOF_MCU_STATE_SLEEP), "set mcu sleep failed\n");

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

void app_cmd_reg_burst_write(const char *cmd)
{
    DTOF_CHECK_WARN(dtof_set_mcu_status_ram(DTOF_MCU_STATE_SLEEP), "set mcu sleep failed\n");

    int reg_addr, reg_num;
    dtof_uint16_t reg_values[255];
    const char *p = cmd;

    if (sscanf(p, "wb,%d,%d", &reg_addr, &reg_num) == 2) {
        // 找到第三个逗号(跳过wb,addr,num)
        const char *values_str = strchr(p, ',');
        if (values_str) values_str = strchr(values_str + 1, ',');
        if (values_str) values_str = strchr(values_str + 1, ',');

        if (values_str) {
            values_str++; // 跳过逗号
            int count = 0;
            const char *token = values_str;
            while (count < reg_num && token && *token) {
                reg_values[count++] = (dtof_uint16_t)atoi(token);
                token = strchr(token, ',');
                if (token) token++;
            }

            // 检查数量是否匹配
            if (count == reg_num) {
                dtof_reg_burst_write(reg_addr, reg_values, reg_num);
                dtof_printf("burst reg write 0x%04x (%d regs): ", reg_addr, reg_num);
                for (int i = 0; i < reg_num; i++) {
                    dtof_printf("%d, ", reg_values[i]);
                }
                dtof_printf("\n");
            } else {
                dtof_printf("param count mismatch (expected %d, got %d)\n", reg_num, count);
            }
        }
    } else {
        dtof_printf("cmd format error. example: wb,1024,3,11,22,33\n");
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
    DTOF_CHECK_RET_VOID(dtof_set_mcu_status_ram(DTOF_MCU_STATE_SLEEP), "mcu sleep failed");
    DTOF_CHECK_RET_VOID(dtof_read_otp(0, otp_data, 128), "read otp failed\n");
    DTOF_CHECK_RET_VOID(dtof_set_mcu_status_ram(DTOF_MCU_STATE_WAKEUP), "mcu wakeup failed");
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
        dtof_uint16_t ft_cali_type = ft_data_read.ft_calibration_type;
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
    dtof_parse_inner_mcu_error_code();
}

void app_cmd_clear_cal_info(const char *cmd) {
    stm32_flash_write_init(DTOF_FT_DATA_FLASH_PAGE, DTOF_FT_DATA_FLASH_PAGE_NUM);
}

void app_cmd_do_ft_calibration(const char *cmd) {
    // stm32_flash_write_init(DTOF_FT_DATA_FLASH_PAGE, DTOF_FT_DATA_FLASH_PAGE_NUM);
    // DTOF_CHECK_WARN(dtof_sensor_init(), "dtof sensor init failed\n");
    dtof_uint16_t ft_cali_type;
    dtof_uint16_t ft_actual_param;; // 可以是otp_ref_spad_mask, 也可以是distance, 目前这种设计下, 不能同时做两个或以上校准

    if (sscanf(cmd, "ft,%hu,%hu", &ft_cali_type, &ft_actual_param) == 2)
    {
        dtof_printf("start ft calibration, type=0x%04x\n", ft_cali_type);
        DTOF_RET ret = dtof_do_ft_calibration(ft_cali_type, ft_actual_param);
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
        DTOF_CHECK_RET_VOID(dtof_set_mcu_status_ram(DTOF_MCU_STATE_SLEEP), "MCU sleep failed");

        dtof_uint16_t ram_data[2];
        dtof_uint16_t dtof_ft_data_start = DTOF_FT_DATA_START + dtof_get_chip_config()->version_lenth - DTOF_FT_DATA_B_OFFSET;

        DTOF_CHECK_RET_VOID(dtof_reg_burst_write(DTOF_WRITE_RAM_START_REG_ADDR, &dtof_ft_data_start, 1), "write ram start addr failed\n");
        DTOF_CHECK_RET_VOID(dtof_reg_burst_read(DTOF_READ_RAM_START_REG_ADDR, ram_data, sizeof(ram_data)/sizeof(ram_data[0])), "read ft data failed\n");

        ram_data[0] = ref_spad;
        DTOF_CHECK_RET_VOID(dtof_reg_burst_write(DTOF_WRITE_RAM_START_REG_ADDR, &dtof_ft_data_start, 1), "write ram start addr failed\n");
        DTOF_CHECK_RET_VOID(dtof_reg_burst_write(DTOF_READ_RAM_START_REG_ADDR, ram_data, sizeof(ram_data)/sizeof(ram_data[0])), "read ft data failed\n");

        DTOF_CHECK_RET_VOID(dtof_set_mcu_status_ram(DTOF_MCU_STATE_WAKEUP), "MCU sleep failed");
    }
}

void app_cmd_dtof_init(const char *cmd) {
    dtof_uint16_t reset_value = 0xffff;
    dtof_uint16_t bypass_value = 0x17b9;

    // bypass inner mcu
    DTOF_CHECK_RET_VOID(dtof_reg_burst_write(DTOF_IO_CTRL_REG_ADDR, &bypass_value, 1), "dtof bypass fail\n");

    // reset chip
    DTOF_CHECK_RET_VOID(dtof_reg_burst_write(DTOF_RESET_REG_ADDR, &reset_value, 1), "chip reset fail\n");

    // reinit
    dtof_init_device_info();
    DTOF_CHECK_RET_VOID(dtof_sensor_init(), "dtof sensor init failed\n");
    dtof_printf("dtof init success\n");
}

void app_cmd_help(const char *cmd) {
    dtof_printf("\n");
    dtof_printf("cmd     function                 format           example\n");
    dtof_printf("s       start distance measure   --               s\n");
    dtof_printf("t       stop distance measure    --               t\n");
    dtof_printf("d       start debug mode         --               d\n");
    dtof_printf("e       start test mode          --               e\n");
    dtof_printf("clear   clear flash cal info     --               clear\n");
    dtof_printf("v       print version info       --               v\n");
    dtof_printf("p       print chip info          --               p\n");
    dtof_printf("init    dtof reinit              --               init\n");
    dtof_printf("ri      read reg from inner mcu  ri,addr          ri,0\n");
    dtof_printf("wi      write reg use inner mcu  wi,addr,data     wi,0,0\n");
    dtof_printf("rb      burst read reg           rb,addr,num      rb,0,2\n");
    dtof_printf("wb      burst write reg          wb,addr,num,data wb,0,2,0,0\n");
    dtof_printf("refspad set refspad              refspad,0        refspad,0\n");
    dtof_printf("ft      do ft calibration        ft,type,param    \n");
    dtof_printf("        type bit0 = 1->binoffset ft,1,dc          ft,1,0\n");
    dtof_printf("        type bit1 = 1->refspad   ft,2,spad_mask   ft,2,254\n");
    dtof_printf("        type bit2 = 1->cg        ft,4,is_to_sky   ft,4,1 (1 to sky, 0 to object)\n");
    dtof_printf("        type bit3 = 1->b         ft,8,distance    ft,8,300\n");
}

void app_cmd_write_ft_data(const char *cmd) {
    if (strncmp(cmd, "wft,", 4) == 0) {
        #define FT_MAX_NUM 39 // type + 34cg+ binoffset + k + b + refspad
        char buf_copy[128];
        strncpy(buf_copy, cmd, sizeof(buf_copy));
        buf_copy[sizeof(buf_copy) - 1] = '\0';

        char *token = strtok(buf_copy, ","); // 第一个 token: "r18"
        int values[FT_MAX_NUM];
        int count = 0;

        while ((token = strtok(NULL, ",")) != NULL) {
            if (count >= FT_MAX_NUM) {
                printf("Too many numbers, need exactly 39\n");
                continue;
            }

            char *endptr;
            long val = strtol(token, &endptr, 10);
            if (*endptr != '\0') {
                printf("Invalid number: %s\n", token);
                continue;
            }

            values[count++] = (int)val;
        }

        if (count < 2) {
            printf("count < 2, = %d\n", count);
            reset_uart_buffer();
            return;
        }

        dtof_uint16_t ft_cali_type;
        dtof_ft_cali_param_t ft_cali_param;
        dtof_bool_t is_legal_ft_data = DTOF_FALSE;

        DTOF_CHECK_RET_VOID(dtof_get_ft_data_from_flash((dtof_uint16_t *)&ft_cali_param, sizeof(dtof_ft_cali_param_t) / sizeof(dtof_uint16_t), &is_legal_ft_data), "get ft data from flash failed\n");

        ft_cali_type = values[0];

        if(DTOF_BIT_GET(ft_cali_type, DTOF_FT_CALIBRATE_BINOFFSET))
        {
            ft_cali_param.dtof_ft_data.bin_offset = values[1];
            printf("set binoffset = %d\n", values[1]);
        }
        if(DTOF_BIT_GET(ft_cali_type, DTOF_FT_CALIBRATE_REFSPAD))
        {
            ft_cali_param.dtof_ft_data.ref_spad = values[1];
            printf("set refspad = %d\n", values[1]);
        }
        if(DTOF_BIT_GET(ft_cali_type, DTOF_FT_CALIBRATE_CG))
        {
            if (count < 18) {
                printf("count < 18, = %d\n", count);
                reset_uart_buffer();
                return;
            }

            printf("set cg: ");
            for (int i = 0; i < (DTOF_AC_NUM + 1); i++)
            {
                ft_cali_param.dtof_ft_data.cg_data[i * 2] = values[i + 1] & 0xff;
                ft_cali_param.dtof_ft_data.cg_data[i * 2 + 1] = (values[i + 1] & 0xff00) >> 8;
                printf("%d, ", values[i + 1]);
            }
            printf("\n");
        }
        if(DTOF_BIT_GET(ft_cali_type, DTOF_FT_CALIBRATE_B))
        {
            ft_cali_param.dtof_ft_data.distance_k = 1197;
            ft_cali_param.dtof_ft_data.distance_b = values[1];
            printf("set b value = %d\n", values[1]);
        }

        if (is_legal_ft_data == DTOF_TRUE) {
            ft_cali_param.ft_calibration_type |= ft_cali_type;
        } else {
            ft_cali_param.ft_calibration_type = ft_cali_type;
        }
        DTOF_CHECK_RET_VOID(dtof_set_ft_data((dtof_uint16_t*)&ft_cali_param), "set ft data failed\n");
        dtof_set_ft_calibration_type(ft_cali_param.ft_calibration_type);
        DTOF_CHECK_RET_VOID(dtof_set_ft_data_to_flash((dtof_uint16_t*)&ft_cali_param, sizeof(ft_cali_param) / sizeof(dtof_uint16_t)), "set ft data to flash failed\n");
    }
}

void app_cmd_get_ram_fsm_status(const char *cmd) {
    DTOF_CHECK_RET_VOID(dtof_parse_inner_mcu_ram_fsm_state(), "dtof_parse_inner_mcu_ram_fsm_state failed\n");
}

void app_cmd_debug_print(const char *cmd) {
    dtof_start_ft_calibrate();
}

void app_cmd_set_frame_rate(const char *cmd) {
    dtof_uint16_t frame_rate;
    if (sscanf(cmd, "rate,%hu", &frame_rate) == 1)
    {
        dtof_printf("set frame rate = %u Hz\n", frame_rate);
        DTOF_CHECK_RET_VOID(dtof_change_frame_rate(frame_rate), "change frame rate failed\n");
    }
}

void app_cmd_change_ram_algo(const char *cmd) {
    dtof_uint16_t ram_algo_flag;
    if (sscanf(cmd, "algo,%hu", &ram_algo_flag) == 1)
    {
        dtof_printf("set ram algo = %u\n", ram_algo_flag);
        DTOF_CHECK_RET_VOID(dtof_switch_ram_algo(ram_algo_flag), "change ram algo failed\n");
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
    { "init",   0, app_cmd_dtof_init },
    { "ri,", 1, app_cmd_reg_read_running },
    { "wi,", 1, app_cmd_reg_write_running },
    { "rb,", 1, app_cmd_reg_burst_read },
    { "wb,", 1, app_cmd_reg_burst_write },
    { "ft,", 1, app_cmd_do_ft_calibration },
    {"refspad,", 1, app_cmd_set_refspad },
    { "h",   0, app_cmd_help },
    { "wft,", 1, app_cmd_write_ft_data },
    { "gm", 0, app_cmd_get_ram_fsm_status },
    { "debug", 0, app_cmd_debug_print },
    { "rate", 1, app_cmd_set_frame_rate },
    { "algo", 1, app_cmd_change_ram_algo },
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
