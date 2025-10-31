#include "inc/dtof_base_type.h"
#include "inc/dtof_global_config.h"
#include "inc/dtof_libc.h"

dtof_chip_config_t *g_chip_config = NULL;
static dtof_chip_config_t chip_configs[] = {
    // L3 芯片配置
    {
        .chip_id          = DTOF_L3_CHIPID,
        .vccio_vsel_auto  = 0x00,
        .vccio_vsel_1_2v  = 0x01,
        .vccio_vsel_1_8v  = 0x02,
        .vccio_vsel_3_3v  = 0x03,
        .chip_uuid        = {0},
    },
    // A05 芯片配置
    {
        .chip_id          = DTOF_A05_CHIPID,
        .vccio_vsel_auto  = 0x04,
        .vccio_vsel_1_2v  = 0x05,
        .vccio_vsel_1_8v  = 0x06,
        .vccio_vsel_3_3v  = 0x07,
        .chip_uuid        = {0},
    },
};
DTOF_RET dtof_find_chip_config(dtof_uint16_t chip_id, dtof_uint8_t *chip_uuid, dtof_uint32_t chip_uuid_len) {
    for (int i = 0; i < sizeof(chip_configs) / sizeof(chip_configs[0]); i++) {
        if (chip_configs[i].chip_id == chip_id) {
            g_chip_config = &chip_configs[i];
            dtof_memcpy((void*)g_chip_config->chip_uuid, chip_uuid, chip_uuid_len);
            return DTOF_RET_SUCCESS;
        }
    }
    return DTOF_RET_FAILED;
}
dtof_chip_config_t *dtof_get_chip_config(void) {
    return g_chip_config;
}