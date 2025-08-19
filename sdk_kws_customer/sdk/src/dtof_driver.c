/*
 * dtof_driver.c
 *
 *  Created on: 2024/8/5
 *      Author: liuzihao
 */
#include "inc/dtof_common.h"
#include "inc/dtof_log.h"
#include "inc/dtof_endian.h"
#include "inc/dtof_driver.h"

#define USE_FT_DATABASE_DATA
#ifdef USE_FT_DATABASE_DATA

#include "inc/dtof_api.h"

#pragma pack(2)
typedef struct
{
    dtof_uint8_t uuid[DTOF_UUID_LENGTH];
    dtof_uint16_t xtalk_data[XTALK_DATA_SIZE];
    dtof_int32_t distance_offset;
} chip_ft_data_t;
#pragma pack()

chip_ft_data_t chip_ft_data[] =
{
    {
        .uuid = {78, 54, 49, 85, 48, 54, 255, 255, 255, 12, 47, 143, 255, 255, 255, 255, },
        .xtalk_data = {5655, 5396, 5910, 6169, 6936, 14635, 14653, 12079, 11311, 9768, 9253, 8482, 7711, 6940, 6169, 5398, 1, 12, },
        .distance_offset = -12,
    },
    {
        .uuid = {78, 54, 49, 85, 48, 54, 255, 255, 255, 12, 55, 143, 255, 255, 255, 255, },
        .xtalk_data = {6682, 6170, 6682, 7196, 7964, 17459, 16969, 13367, 13624, 11826, 11053, 10025, 9254, 8226, 7454, 6427, 1, 11, },
        .distance_offset = 6,
    },
    {
        .uuid = {78, 54, 49, 85, 48, 54, 255, 255, 255, 12, 46, 143, 255, 255, 255, 255, },
        .xtalk_data = {5654, 4629, 5651, 6168, 6680, 16430, 14147, 10538, 10540, 8997, 8482, 7711, 7197, 6426, 5912, 5142, 1, 11, },
        .distance_offset = -2,
    },
    {
        .uuid = {78, 54, 49, 85, 48, 54, 255, 255, 255, 12, 36, 143, 255, 255, 255, 255, },
        .xtalk_data = {13109, 12338, 13363, 14136, 14904, 21062, 19795, 16709, 15678, 15164, 14650, 14136, 13622, 12852, 12337, 11823, 1, 11, },
        .distance_offset = 4,
    },
    {
        .uuid = {78, 54, 49, 85, 48, 54, 255, 255, 255, 12, 43, 143, 255, 255, 255, 255, },
        .xtalk_data = {6168, 5142, 5910, 6425, 6680, 12837, 12340, 9768, 9768, 8483, 7968, 7454, 6940, 6426, 5912, 5398, 1, 11, },
        .distance_offset = 7,
    },
    {
        .uuid = {78, 54, 49, 85, 48, 54, 255, 255, 255, 12, 30, 143, 255, 255, 255, 255, },
        .xtalk_data = {64627, 4371, 4369, 4882, 5396, 14115, 14910, 9771, 9766, 8228, 7197, 6683, 6169, 5655, 4885, 4370, 1, 10, },
        .distance_offset = 10,
    },
    {
        .uuid = {78, 54, 49, 85, 48, 54, 255, 255, 255, 12, 59, 143, 255, 255, 255, 255, },
        .xtalk_data = {11308, 10028, 10279, 11563, 11821, 20538, 22873, 18255, 18761, 16966, 15936, 14908, 13624, 12595, 11567, 10539, 1, 10, },
        .distance_offset = -2,
    },
    {
        .uuid = {78, 54, 49, 85, 48, 54, 255, 255, 255, 12, 70, 143, 255, 255, 255, 255, },
        .xtalk_data = {12593, 11567, 12335, 12851, 13106, 20545, 20051, 16708, 17221, 15680, 14907, 14137, 13366, 12595, 12080, 11309, 1, 11, },
        .distance_offset = 0,
    },
    {
        .uuid = {78, 54, 49, 85, 48, 54, 255, 255, 255, 12, 61, 143, 255, 255, 255, 255, },
        .xtalk_data = {13109, 12083, 12334, 13363, 14133, 20287, 21845, 18511, 19016, 17737, 16707, 15679, 14908, 13880, 13108, 12081, 1, 10, },
        .distance_offset = 4,
    },
    {
        .uuid = {78, 54, 49, 85, 48, 54, 255, 255, 255, 12, 48, 143, 255, 255, 255, 255, },
        .xtalk_data = {8481, 7455, 8223, 8995, 9507, 16948, 16453, 13110, 12853, 11568, 10795, 10281, 9510, 8996, 8225, 7455, 1, 11, },
        .distance_offset = 1,
    },
    {
        .uuid = {78, 54, 49, 85, 48, 54, 255, 255, 255, 12, 65, 143, 255, 255, 255, 255, },
        .xtalk_data = {14650, 13879, 14905, 15934, 15934, 22345, 23387, 19025, 18248, 17478, 16963, 16192, 15421, 14651, 13880, 13365, 1, 11, },
        .distance_offset = 3,
    },
    {
        .uuid = {78, 54, 49, 85, 48, 54, 255, 255, 255, 12, 53, 143, 255, 255, 255, 255, },
        .xtalk_data = {12593, 11568, 12334, 13364, 13876, 22343, 21337, 17992, 18506, 16965, 16193, 15165, 14394, 13366, 12594, 11567, 1, 11, },
        .distance_offset = 7,
    },
    {
        .uuid = {78, 54, 49, 85, 48, 54, 255, 255, 255, 12, 31, 143, 255, 255, 255, 255, },
        .xtalk_data = {11052, 10282, 11562, 12078, 12079, 18491, 18507, 15423, 15679, 14395, 13879, 13109, 12338, 11567, 11052, 10281, 1, 11, },
        .distance_offset = 15,
    },
    {
        .uuid = {78, 54, 49, 85, 48, 54, 255, 255, 255, 12, 25, 143, 255, 255, 255, 255, },
        .xtalk_data = {4369, 4112, 4368, 4882, 5395, 12322, 13366, 10282, 10025, 8484, 7968, 7198, 6427, 5656, 5141, 4370, 1, 12, },
        .distance_offset = 9,
    },
    {
        .uuid = {78, 54, 49, 85, 48, 54, 255, 255, 255, 12, 45, 143, 255, 255, 255, 255, },
        .xtalk_data = {20304, 17996, 19529, 21073, 21072, 25947, 25959, 22622, 22103, 21333, 20818, 20304, 19533, 19019, 18505, 17734, 1, 11, },
        .distance_offset = 9,
    },
};

static void dtof_find_chip_ft_data_in_database(dtof_uint8_t *uuid, dtof_uint8_t uuid_len, dtof_uint8_t **sensor_ft_data_p)
{
    dtof_int32_t sensor_index;
    dtof_bool_t is_find_sensor = DTOF_FALSE;

    for (sensor_index = 0; sensor_index < sizeof(chip_ft_data) / sizeof(chip_ft_data_t); sensor_index++)
    {
        if (memcmp(uuid, chip_ft_data[sensor_index].uuid, uuid_len) == 0)
        {
            is_find_sensor = DTOF_TRUE;
            *sensor_ft_data_p = (dtof_uint8_t*)&chip_ft_data[sensor_index];
            break;
        }
    }

    if(!(is_find_sensor)){
        *sensor_ft_data_p = (dtof_uint8_t*)&chip_ft_data[0];
        DTOF_LOG("can not find uuid in database, use default data\n");
    }

    return;
}

#endif

static void dtof_convert_endian(uint16_t *data, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++) {
        data[i] = DTOF_SWAP16(data[i]);
    }
}

/**
 * @brief burst read register
 * @param[in] reg_addr register address
 * @param[out] reg_data_p register data pointer
 * @param[in] len data length
 * @return DTOF_RET_SUCCESS success, DTOF_RET_FAILED fail
 */
DTOF_RET DTOF_WEAK dtof_reg_burst_read(dtof_uint8_t device_id, dtof_uint8_t reg_addr, dtof_uint16_t *reg_data_p, dtof_uint16_t len)
{
    DTOF_LOG("reg burst read use weak func!\n");

    // 这里写读取寄存器的代码

    dtof_convert_endian(reg_data_p, len);

    return DTOF_RET_SUCCESS;
}

/**
 * @brief burst write to register
 * @param[in] reg_addr register address
 * @param[in] reg_data_p pointer to the data to be written
 * @param[in] len length of the data
 * @return DTOF_RET_SUCCESS on success, DTOF_RET_FAILED on failure
 */
DTOF_RET DTOF_WEAK dtof_reg_burst_write(dtof_uint8_t device_id, dtof_uint8_t reg_addr, dtof_uint16_t *reg_data_p, dtof_uint16_t len)
{
    dtof_convert_endian(reg_data_p, len);

    DTOF_LOG("reg burst write use weak func!\n");

    // 这里写写入寄存器的代码

    dtof_convert_endian(reg_data_p, len);
    return DTOF_RET_SUCCESS;
}

DTOF_RET DTOF_WEAK dtof_reg_burst_write_burn(dtof_uint8_t device_id, dtof_uint8_t reg_addr, const dtof_uint16_t *reg_data_p, dtof_uint16_t len)
{
    DTOF_LOG("reg burst write burn use weak func!\n");

    return DTOF_RET_SUCCESS;
}

DTOF_RET DTOF_WEAK dtof_get_distance_offset_from_flash(dtof_uint8_t device_id, dtof_int32_t *distance_offset)
{
#ifdef USE_FT_DATABASE_DATA
    dtof_uint8_t uuid[DTOF_UUID_LENGTH];
    dtof_uint8_t *sensor_ft_data_p;
    DTOF_CHECK_RET(dtof_get_uuid(device_id, uuid, DTOF_UUID_LENGTH), "get uuid failed\n");
    dtof_find_chip_ft_data_in_database(uuid, DTOF_UUID_LENGTH, &sensor_ft_data_p);
    *distance_offset = ((chip_ft_data_t*)sensor_ft_data_p)->distance_offset;
#else
    *distance_offset = 0;
#endif

    DTOF_LOG("get distance offset use weak func!\n");

    return DTOF_RET_SUCCESS;
}

DTOF_RET DTOF_WEAK dtof_set_distance_offset_to_flash(dtof_uint8_t device_id, dtof_int32_t distance_offset)
{
    DTOF_LOG("set distance offset use weak func!\n");

    return DTOF_RET_SUCCESS;
}

DTOF_RET DTOF_WEAK dtof_get_xtalk_data_from_flash(dtof_uint8_t device_id, dtof_uint16_t *xtalk_data)
{
#ifdef USE_FT_DATABASE_DATA
    dtof_uint8_t uuid[DTOF_UUID_LENGTH];
    dtof_uint8_t *sensor_ft_data_p;
    DTOF_CHECK_RET(dtof_get_uuid(device_id, uuid, DTOF_UUID_LENGTH), "get uuid failed\n");
    dtof_find_chip_ft_data_in_database(uuid, DTOF_UUID_LENGTH, &sensor_ft_data_p);
    dtof_memcpy(xtalk_data, ((chip_ft_data_t*)sensor_ft_data_p)->xtalk_data, XTALK_DATA_SIZE * sizeof(dtof_uint16_t));
#else
    // xtalk_data是uint16_t类型数组指针, 大小为18, 这里给默认值
    dtof_uint16_t xtalk_data_default[XTALK_DATA_SIZE] = {11, 258, 257, 514, 514, 514, 1284, 1285, 772, 1028, 771, 771, 514, 514, 514, 514, 258, 1};
    dtof_memcpy(xtalk_data, xtalk_data_default, sizeof(xtalk_data_default));
#endif
    DTOF_LOG("get xtalk data use weak func!\n");

    return DTOF_RET_SUCCESS;
}

DTOF_RET DTOF_WEAK dtof_set_xtalk_data_from_flash(dtof_uint8_t device_id, dtof_uint16_t *xtalk_data)
{
    DTOF_LOG("set xtalk data use weak func!\n");

    for(dtof_uint16_t i = 0; i < XTALK_DATA_SIZE; i++)
    {
        printf("%d, ", xtalk_data[i]);
    }
    printf("\n");

    return DTOF_RET_SUCCESS;
}



/**
 * @brief IO interaction
 * @param[in] cmd command value
 * @param[in] value argument value
 * @return DTOF_RET_SUCCESS success, DTOF_RET_FAILED fail
 * Send a command to the sensor and wait for the response.
 */
DTOF_RET dtof_io_interaction(dtof_uint8_t device_id, dtof_uint16_t cmd, dtof_uint16_t value)
{
    DTOF_RET ret;
    dtof_uint16_t reg_data = PACK_IO_INTERACTION_DATA(cmd, value);
    ret = dtof_reg_burst_write(device_id, DTOF_IO_CTRL_REG_ADDR, &reg_data, 1);
    return ret;
}

/**
 * @brief Sets the interrupt flag to the specified value.
 * @param[in] interrupt flag
 */
void DTOF_WEAK dtof_set_interrupt_flag(dtof_bool_t flag)
{
    return;
}

/**
 * @brief Get the interrupt flag value.
 * @return DTOF_TRUE if there is an interrupt, DTOF_FALSE otherwise.
 */
dtof_bool_t DTOF_WEAK dtof_get_interrupt_flag(void)
{
    return 0;
}

/**
 * @brief Sleep for a specified number of milliseconds.
 * @param[in] time Number of milliseconds to sleep.
 */
void DTOF_WEAK dtof_sleep_ms(dtof_uint32_t time)
{
    return;
}

char DTOF_WEAK uart_getchar(void) {
    DTOF_LOG("uart getchar use weak func!\n");
    return 0;
}
