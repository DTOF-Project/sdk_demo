#ifndef _DTOF_GLOBAL_CONFIG_H_
#define _DTOF_GLOBAL_CONFIG_H_

#include "inc/dtof_base_type.h"
#include "inc/dtof_common.h"

#define DTOF_L3_CHIPID  0x4120
#define DTOF_A05_CHIPID 0x0001

#pragma pack(2)

typedef struct {
    uint16_t chip_id;           // 芯片ID
    uint16_t vccio_vsel_auto;   // 自动电压选择
    uint16_t vccio_vsel_1_2v;   // 1.2V 电压选择
    uint16_t vccio_vsel_1_8v;   // 1.8V 电压选择
    uint16_t vccio_vsel_3_3v;   // 3.3V 电压选择
    dtof_uint8_t chip_uuid[DTOF_UUID_LENGTH];
} dtof_chip_config_t;

#pragma pack()

dtof_chip_config_t *dtof_get_chip_config(void);
DTOF_RET dtof_find_chip_config(dtof_uint16_t chip_id, dtof_uint8_t *chip_uuid, dtof_uint32_t chip_uuid_len);

#endif