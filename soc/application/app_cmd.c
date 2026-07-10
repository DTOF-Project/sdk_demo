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
#include "application/inc/app_heatmap.h"

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

DTOF_RET dtof_set_auto_switch_mode(dtof_uint16_t auto_switch_mode)
{
#define ENABLE_AUTO_SWITCH_MODE 1
#define DISABLE_AUTO_SWITCH_MODE 0
#define DTOF_AUTO_SWITCH_MODE 0X0888          // 低12bit为0x888时, 代表要启用/禁用自动切换环境光模式
    dtof_uint16_t dtof_auto_switch_mode_value;

    if(auto_switch_mode != ENABLE_AUTO_SWITCH_MODE && auto_switch_mode != DISABLE_AUTO_SWITCH_MODE) {
        dtof_printf("invalid auto switch mode: %d\n", auto_switch_mode);
        return DTOF_RET_FAILED;
    }

    dtof_auto_switch_mode_value = DTOF_AUTO_SWITCH_MODE | (auto_switch_mode << 12);
    dtof_printf("set auto switch mode: 0x%04x\n", dtof_auto_switch_mode_value);
    DTOF_CHECK_RET(dtof_write_reg_running(DTOF_FRAME_CONTROL_REG, dtof_auto_switch_mode_value), "dtof swicth frame rate failed\n");
    DTOF_CHECK_RET(dtof_wait_running_mode_switch_done(), "wait running mode switch done failed\n");
    return DTOF_RET_SUCCESS;
}

DTOF_RET dtof_start_distance_measure_debug_mode(void)
{
    DTOF_CHECK_RET(dtof_set_mcu_status(DTOF_MCU_STATE_SLEEP_DIRECT), "set mcu sleep failed\n");

    uint16_t start_flag = DTOF_START_DISATNCE_MODE;
    dtof_reg_burst_write(DTOF_FRAME_CONTROL_REG, &start_flag, 1);

    DTOF_CHECK_RET(dtof_set_mcu_status(DTOF_MCU_STATE_WAKEUP), "wakeup mcu failed\n");
    return DTOF_RET_SUCCESS;
}


// ========== 命令处理函数 ==========
void app_cmd_start_distance_measure(const char *cmd) {
    app_set_distance_mode(DISTANCE_NORMAL_MODE);
    DTOF_CHECK_WARN(dtof_sensor_init(), "dtof sensor init failed\n");
    DTOF_CHECK_WARN(dtof_start_distance_measure(), "dtof start distance mode failed\n");
}

void app_cmd_stop_distance_measure(const char *cmd) {
    // app_set_distance_mode(DISTANCE_UNKNOWN_MODE);
    DTOF_CHECK_WARN(dtof_stop_distance_measure(), "dtof stop distance mode failed\n");
    // dtof_sleep_ms(33); //TODO: 待优化, 要等可能存在的上一帧跑完
    // dtof_set_interrupt_flag(DTOF_FALSE);
    // app_set_distance_mode(DISTANCE_UNKNOWN_MODE);
}

void app_cmd_start_distance_measure_debug_mode(const char *cmd) {
    app_set_distance_mode(DISTANCE_DEBUG_MODE);
    DTOF_CHECK_WARN(dtof_sensor_init(), "dtof sensor init failed\n");
    DTOF_CHECK_WARN(dtof_enable_distance_debug_mode(), "enable debug mode failed\n");
    DTOF_CHECK_WARN(dtof_start_distance_measure_debug_mode(), "dtof start distance mode failed\n");
}

void app_cmd_start_distance_measure_test_mode(const char *cmd) {
    app_set_distance_mode(DISTANCE_TEST_MODE);
    DTOF_CHECK_WARN(dtof_sensor_init(), "dtof sensor init failed\n");
    DTOF_CHECK_WARN(dtof_enable_distance_debug_mode(), "enable debug mode failed\n");
    DTOF_CHECK_WARN(dtof_start_distance_measure_debug_mode(), "dtof start distance mode failed\n");
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
    DTOF_LOG("lib version: %s\n", dtof_get_lib_version());
    DTOF_LOG("ram version: %d\n", DTOF_SWAP16(dtof_get_chip_version()));
    DTOF_LOG("chip id: 0x%04x\n", dtof_get_chip_config()->chip_id);
    DTOF_LOG("chip uuid: ");
    for (int i = 0; i < DTOF_UUID_LENGTH; i++)
    {
        dtof_printf("%d, ", dtof_get_chip_config()->chip_uuid[i]);
    }
}

void print_ft_data_from_flash(dtof_run_mode_e run_mode)
{
    dtof_ft_cali_param_t ft_data_read;
    dtof_bool_t is_legal_data = DTOF_FALSE;
    dtof_get_ft_data_from_flash_multi_mode((dtof_uint16_t*)&ft_data_read, sizeof(dtof_ft_cali_param_t)/sizeof(dtof_uint16_t), run_mode, &is_legal_data);
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
            // dtof_printf("distance_k=%d, distance_b=%d\n", ft_data_read.dtof_ft_data.distance_k, ft_data_read.dtof_ft_data.distance_b);
            dtof_printf("distance_k=%d\r\n",ft_data_read.dtof_ft_data.distance_k);
            for (int i = 0; i < CAL_QUANTITY; i++){
                dtof_printf("distance_b[%d] = %d\r\n",i,ft_data_read.dtof_ft_data.distance_b[i]);
            }
        }
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

    dtof_parse_inner_mcu_error_code();

    dtof_run_mode_e running_mode;
    DTOF_CHECK_WARN(dtof_read_running_mode(&running_mode), "read running mode failed\n");
    dtof_printf("running mode = %d(0:30Hz, 1:120Hz LP, 2:120Hz LLP)\n", running_mode);

    dtof_printf("\nft data 30Hz:\n");
    print_ft_data_from_flash(RUNNING_MODE_30HZ);

    dtof_printf("\nft data 120Hz LP:\n");
    print_ft_data_from_flash(RUNNING_MODE_120HZ_LP);

    dtof_printf("\nft data 120Hz LLP:\n");
    print_ft_data_from_flash(RUNNING_MODE_120HZ_LLP);

    dtof_sleep_ms(33); //TODO: 待优化
}

void app_cmd_print_ram_ft_data(const char *cmd) {
    // DTOF_CHECK_RET_VOID(dtof_get_ft_slot_start_addr(&dtof_ft_data_start, DTOF_INNER_FT_SLOT_30HZ), "get ft slot addr failed\n");

    dtof_uint16_t dtof_ft_data_start;
    dtof_uint16_t dtof_ft_data_inner[DTOF_FT_DATA_LEN*4];
    dtof_ft_data_t *dtof_ft_data_inner_p = (dtof_ft_data_t *)dtof_ft_data_inner;

    DTOF_CHECK_RET_VOID(dtof_set_mcu_status_ram(DTOF_MCU_STATE_SLEEP_DIRECT), "set mcu sleep failed\n");
    // DTOF_CHECK_RET_VOID(dtof_get_active_ft_start_addr(&dtof_ft_data_start), "get active ft addr failed\n");
    DTOF_CHECK_RET_VOID(dtof_get_ft_slot_start_addr(&dtof_ft_data_start, DTOF_INNER_FT_SLOT_30HZ), "get ft slot addr failed\n");

    DTOF_CHECK_RET_VOID(dtof_reg_burst_write(DTOF_WRITE_RAM_START_REG_ADDR, &dtof_ft_data_start, 1), "write ram start addr failed\n");
    DTOF_CHECK_RET_VOID(dtof_reg_burst_read(DTOF_READ_RAM_START_REG_ADDR, dtof_ft_data_inner, DTOF_FT_DATA_LEN), "read ft data failed\n");

    printf("ft data:\n");
    for(int i = 0; i < DTOF_FT_DATA_LEN; i++)
    {
        printf("%d, ", dtof_ft_data_inner[i]);
    }
    printf("\n");


    DTOF_CHECK_RET_VOID(dtof_get_ft_slot_start_addr(&dtof_ft_data_start, DTOF_INNER_FT_SLOT_120HZ_LP), "get ft slot addr failed\n");

    DTOF_CHECK_RET_VOID(dtof_reg_burst_write(DTOF_WRITE_RAM_START_REG_ADDR, &dtof_ft_data_start, 1), "write ram start addr failed\n");
    DTOF_CHECK_RET_VOID(dtof_reg_burst_read(DTOF_READ_RAM_START_REG_ADDR, dtof_ft_data_inner, DTOF_FT_DATA_LEN), "read ft data failed\n");

    printf("ft data:\n");
    for(int i = 0; i < DTOF_FT_DATA_LEN; i++)
    {
        printf("%d, ", dtof_ft_data_inner[i]);
    }
    printf("\n");


    DTOF_CHECK_RET_VOID(dtof_get_ft_slot_start_addr(&dtof_ft_data_start, DTOF_INNER_FT_SLOT_120HZ_LLP), "get ft slot addr failed\n");

    DTOF_CHECK_RET_VOID(dtof_reg_burst_write(DTOF_WRITE_RAM_START_REG_ADDR, &dtof_ft_data_start, 1), "write ram start addr failed\n");
    DTOF_CHECK_RET_VOID(dtof_reg_burst_read(DTOF_READ_RAM_START_REG_ADDR, dtof_ft_data_inner, DTOF_FT_DATA_LEN), "read ft data failed\n");

    printf("ft data:\n");
    for(int i = 0; i < DTOF_FT_DATA_LEN; i++)
    {
        printf("%d, ", dtof_ft_data_inner[i]);
    }
    printf("\n");


    DTOF_CHECK_RET_VOID(dtof_get_ft_slot_start_addr(&dtof_ft_data_start, DTOF_INNER_FT_SLOT_ACTIVE), "get ft slot addr failed\n");

    DTOF_CHECK_RET_VOID(dtof_reg_burst_write(DTOF_WRITE_RAM_START_REG_ADDR, &dtof_ft_data_start, 1), "write ram start addr failed\n");
    DTOF_CHECK_RET_VOID(dtof_reg_burst_read(DTOF_READ_RAM_START_REG_ADDR, dtof_ft_data_inner, DTOF_FT_DATA_LEN), "read ft data failed\n");

    printf("ft data:\n");
    for(int i = 0; i < DTOF_FT_DATA_LEN; i++)
    {
        printf("%d, ", dtof_ft_data_inner[i]);
    }
    printf("\n");

    DTOF_CHECK_RET_VOID(dtof_set_mcu_status_ram(DTOF_MCU_STATE_WAKEUP), "wakeup mcu failed\n");
}

void app_cmd_clear_cal_info(const char *cmd) {
    stm32_flash_write_init(DTOF_FT_DATA_FLASH_PAGE, RUNNING_MODE_NUM);
}

void app_cmd_do_ft_calibration(const char *cmd) {
    // stm32_flash_write_init(DTOF_FT_DATA_FLASH_PAGE, DTOF_FT_DATA_FLASH_PAGE_NUM);
    // DTOF_CHECK_WARN(dtof_sensor_init(), "dtof sensor init failed\n");
    dtof_uint16_t ft_cali_type;
    dtof_uint16_t ft_actual_param; // 可以是otp_ref_spad_mask, 也可以是distance, 目前这种设计下, 不能同时做两个或以上校准

    if (sscanf(cmd, "ft,%hu,%hu", &ft_cali_type, &ft_actual_param) == 2)
    {
        dtof_printf("start ft calibration, type=0x%04x\n", ft_cali_type);

        ft_cali_type = BIT_POS_LOOP(ft_cali_type);

        // for zhumi: 校准低功耗和低低功耗的数据
        DTOF_CHECK_RET_VOID(dtof_do_ft_calibration_all_mode(ft_cali_type, ft_actual_param), "ft calibration all mode failed\n");

        dtof_printf("120Hz LP FT success:\n");
        print_ft_data_from_flash(RUNNING_MODE_120HZ_LP);

        dtof_printf("\n120Hz LLP FT success:\n");
        print_ft_data_from_flash(RUNNING_MODE_120HZ_LLP);
    }
}

void app_cmd_set_spad(const char *cmd) {
    dtof_uint16_t reg_addr, reg_value;
    if (sscanf(cmd, "spad,%hu,%hu", &reg_addr, &reg_value) == 2)
    {
        #define DTOF_SPAD_REG_START_ADDR 204
        #define DTOF_SPAD_REG_END_ADDR 208
        #define DTOF_FT_DATA_START 0x2000
        #define DTOF_FT_DATA_B_OFFSET 4

        if(reg_addr < DTOF_SPAD_REG_START_ADDR || reg_addr > DTOF_SPAD_REG_END_ADDR)
        {
            dtof_printf("invalid spad reg addr %hu, should be between %d and %d\n", reg_addr, DTOF_SPAD_REG_START_ADDR, DTOF_SPAD_REG_END_ADDR);
            return;
        }
        dtof_printf("set spad reg %hu = %hu\n", reg_addr, reg_value);

        // 关闭软件低功耗, 目的是使用PLL时钟
        DTOF_CHECK_RET_VOID(dtof_start_ft_calibrate(), "ft start failed\n");
        DTOF_CHECK_RET_VOID(dtof_set_mcu_status_ram(DTOF_MCU_STATE_SLEEP_DIRECT), "MCU sleep failed");

        if(reg_addr != DTOF_SPAD_REG_END_ADDR)
        {
            // main spad mask
            DTOF_CHECK_RET_VOID(hal_spad_mask_config(reg_addr, reg_value), "spad mask config failed\n");
            DTOF_CHECK_RET_VOID(dtof_set_mcu_status_ram(DTOF_MCU_STATE_WAKEUP), "MCU sleep failed");
            DTOF_CHECK_RET_VOID(dtof_stop_distance_measure(), "dtof stop distance mode failed\n");
        } else {
            // ref spad mask
            dtof_run_mode_e running_mode;
            dtof_ft_cali_param_t ft_cali_param;
            dtof_bool_t is_legal_ft_data = DTOF_FALSE;
            dtof_uint16_t ram_data[2];
            dtof_uint16_t dtof_ft_data_start = DTOF_FT_DATA_START + dtof_get_chip_config()->version_lenth - DTOF_FT_DATA_B_OFFSET;

            DTOF_CHECK_RET_VOID(dtof_reg_burst_write(DTOF_WRITE_RAM_START_REG_ADDR, &dtof_ft_data_start, 1), "write ram start addr failed\n");
            DTOF_CHECK_RET_VOID(dtof_reg_burst_read(DTOF_READ_RAM_START_REG_ADDR, ram_data, sizeof(ram_data)/sizeof(ram_data[0])), "read ft data failed\n");

            ram_data[0] = reg_value;
            DTOF_CHECK_RET_VOID(dtof_reg_burst_write(DTOF_WRITE_RAM_START_REG_ADDR, &dtof_ft_data_start, 1), "write ram start addr failed\n");
            DTOF_CHECK_RET_VOID(dtof_reg_burst_write(DTOF_READ_RAM_START_REG_ADDR, ram_data, sizeof(ram_data)/sizeof(ram_data[0])), "read ft data failed\n");

            DTOF_CHECK_RET_VOID(dtof_set_mcu_status_ram(DTOF_MCU_STATE_WAKEUP), "MCU sleep failed");
            DTOF_CHECK_RET_VOID(dtof_reload_ft_data(), "reload ft data failed\n");

            DTOF_CHECK_RET_VOID(dtof_read_running_mode(&running_mode), "read running mode failed\n");
            DTOF_CHECK_RET_VOID(dtof_get_ft_data_from_flash_multi_mode((dtof_uint16_t *)&ft_cali_param, sizeof(dtof_ft_cali_param_t) / sizeof(dtof_uint16_t), running_mode, &is_legal_ft_data), "get ft data from flash failed\n");
            DTOF_CHECK_RET_VOID(dtof_get_ft_data_from_ram(&ft_cali_param.dtof_ft_data), "get ft data from ram failed\n");
            if (is_legal_ft_data == DTOF_TRUE)
            {
                ft_cali_param.ft_calibration_type |= DTOF_BIT(DTOF_FT_CALIBRATE_REFSPAD);
            }
            else
            {
                ft_cali_param.ft_calibration_type = DTOF_BIT(DTOF_FT_CALIBRATE_REFSPAD);
            }
            dtof_set_ft_calibration_type(running_mode, ft_cali_param.ft_calibration_type);
            DTOF_CHECK_RET_VOID(dtof_set_ft_data_to_flash_multi_mode((dtof_uint16_t *)&ft_cali_param, sizeof(ft_cali_param) / sizeof(dtof_uint16_t), running_mode), "set ft data to flash failed\n");
            DTOF_CHECK_RET_VOID(dtof_sync_ft_slot_by_mode(running_mode, &ft_cali_param.dtof_ft_data), "sync ft slot failed\n");
        }
    }
}

void app_cmd_set_auto_switch_mode(const char *cmd) {
    dtof_uint16_t auto_switch_mode;
    if (sscanf(cmd, "auto,%hu", &auto_switch_mode) == 1)
    {
        dtof_printf("set auto switch mode = %hu\n", auto_switch_mode);
        dtof_set_auto_switch_mode(auto_switch_mode);
    }
}


void app_cmd_set_running_mode(const char *cmd) {
    dtof_uint16_t running_mode;
    if (sscanf(cmd, "mode,%hu", &running_mode) == 1)
    {
        switch(running_mode)
        {
            case RUNNING_MODE_30HZ:
            {
                DTOF_CHECK_RET_VOID(dtof_switch_running_mode(RUNNING_MODE_30HZ), "switch running mode failed\n");
                dtof_printf("set running mode = 30Hz\n");
                break;
            }
            case RUNNING_MODE_120HZ_LP:
            {
                DTOF_CHECK_RET_VOID(dtof_switch_running_mode(RUNNING_MODE_120HZ_LP), "switch running mode failed\n");
                dtof_printf("set running mode = 120Hz LP\n");
                break;
            }
            case RUNNING_MODE_120HZ_LLP:
            {
                DTOF_CHECK_RET_VOID(dtof_switch_running_mode(RUNNING_MODE_120HZ_LLP), "switch running mode failed\n");
                dtof_printf("set running mode = 120Hz LLP\n");
                break;
            }
            default:
            {
                dtof_printf("unsupported running mode %u\n", running_mode);
                return;
            }
        }
    }
}

void app_cmd_write_ft_data(const char *cmd) {
    if (strncmp(cmd, "wft,", 4) == 0) {
        #define FT_MAX_NUM 43 // runmode + type + mode + 34cg+ binoffset + k + b + refspad
        char buf_copy[128];
        strncpy(buf_copy, cmd, sizeof(buf_copy));
        buf_copy[sizeof(buf_copy) - 1] = '\0';

        char *token = strtok(buf_copy, ","); // 第一个 token: "r18"
        int values[FT_MAX_NUM];
        int count = 0;

        while ((token = strtok(NULL, ",")) != NULL) {
            if (count >= FT_MAX_NUM) {
                printf("Too many numbers, need exactly 40\n");
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

        dtof_run_mode_e run_mode;
        dtof_uint16_t ft_cali_type;
        dtof_ft_cali_param_t ft_cali_param;
        dtof_bool_t is_legal_ft_data = DTOF_FALSE;

        run_mode = values[0];
        ft_cali_type = values[1];

        DTOF_CHECK_RET_VOID(dtof_get_ft_data_from_flash_multi_mode((dtof_uint16_t *)&ft_cali_param, sizeof(dtof_ft_cali_param_t) / sizeof(dtof_uint16_t), run_mode, &is_legal_ft_data), "get ft data from flash failed\n");

        if(DTOF_BIT_GET(ft_cali_type, DTOF_FT_CALIBRATE_BINOFFSET))
        {
            ft_cali_param.dtof_ft_data.bin_offset = values[2];
            printf("set binoffset = %d\n", values[2]);
        }
        if(DTOF_BIT_GET(ft_cali_type, DTOF_FT_CALIBRATE_REFSPAD))
        {
            ft_cali_param.dtof_ft_data.ref_spad = values[2];
            printf("set refspad = %d\n", values[2]);
        }
        if(DTOF_BIT_GET(ft_cali_type, DTOF_FT_CALIBRATE_CG))
        {
            if (count < 19) {
                printf("count < 19, = %d\n", count);
                reset_uart_buffer();
                return;
            }

            printf("set cg: ");
            for (int i = 0; i < (DTOF_AC_NUM + 1); i++)
            {
                ft_cali_param.dtof_ft_data.cg_data[i * 2] = values[i + 2] & 0xff;
                ft_cali_param.dtof_ft_data.cg_data[i * 2 + 1] = (values[i + 2] & 0xff00) >> 8;
                printf("%d, ", values[i + 2]);
            }
            printf("\n");
        }
        if(DTOF_BIT_GET(ft_cali_type, DTOF_FT_CALIBRATE_B))
        {
            if (count < 4)
            {
                printf("need: runmode,type,index,b\n");
                return;
            }

            uint8_t b_index[4] = values[2];

            if (b_index >= CAL_QUANTITY)
            {
                printf("invalid b index = %d\n", b_index);
                return;
            }
            ft_cali_param.dtof_ft_data.distance_k = 1197;
            // ft_cali_param.dtof_ft_data.distance_b = values[2];
            ft_cali_param.dtof_ft_data.distance_b[b_index] = values[3];
            // printf("set b value = %d\n", values[2]);
            printf("set distance_b[%d] = %d\n",b_index,values[3]);
        }

        if (is_legal_ft_data == DTOF_TRUE) {
            ft_cali_param.ft_calibration_type |= ft_cali_type;
        } else {
            ft_cali_param.ft_calibration_type = ft_cali_type;
        }
        dtof_set_ft_calibration_type(run_mode, ft_cali_param.ft_calibration_type);
        DTOF_CHECK_RET_VOID(dtof_set_ft_data_to_flash_multi_mode((dtof_uint16_t*)&ft_cali_param, sizeof(ft_cali_param) / sizeof(dtof_uint16_t), run_mode), "set ft data to flash failed\n");
        DTOF_CHECK_RET_VOID(dtof_sync_ft_slot_by_mode(run_mode, &ft_cali_param.dtof_ft_data), "sync ft slot failed\n");
    }
}

void app_cmd_reg_burst_read_d(const char *cmd) {

    int reg_addr, reg_num;
    dtof_uint16_t reg_max[255];

    if (sscanf(cmd, "rr,%d,%d", &reg_addr, &reg_num) == 2) {
        dtof_reg_burst_read(reg_addr, reg_max, reg_num);
        dtof_printf("burst reg read 0x%04x:\n", reg_addr);
        for (int i = 0; i < reg_num; i++) {
            dtof_printf("%d, ", reg_max[i]);
        }
        dtof_printf("\n");
    }

}

void app_cmd_heatmap_output(const char *cmd) {
    app_heatmap_output();
}

void app_cmd_test_base_function(const char *cmd) {
    extern DTOF_RET dtof_diag_test(void);
    dtof_diag_test();
}

cmd_entry_t cmd_table[] = {
    { "s",   0, app_cmd_start_distance_measure },
    { "t",   0, app_cmd_stop_distance_measure },
    { "d",   0, app_cmd_start_distance_measure_debug_mode },
    { "e",   0, app_cmd_start_distance_measure_test_mode },
    { "clear", 0, app_cmd_clear_cal_info },
    { "v",   0, app_cmd_get_version },
    { "p",   0, app_cmd_print_chip_info },
    {"ram", 0, app_cmd_print_ram_ft_data },
    { "heatmap",   0, app_cmd_heatmap_output },
    {"test", 0, app_cmd_test_base_function },
    { "ri,", 1, app_cmd_reg_read_running },
    { "wi,", 1, app_cmd_reg_write_running },
    { "rb,", 1, app_cmd_reg_burst_read },
    { "rr,", 1, app_cmd_reg_burst_read_d },
    { "ft,", 1, app_cmd_do_ft_calibration },
    {"spad,", 1, app_cmd_set_spad },
    {"mode,", 1, app_cmd_set_running_mode },
    {"wft,", 1, app_cmd_write_ft_data },
    {"auto,", 1, app_cmd_set_auto_switch_mode },
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
