#include "inc/dtof_base_type.h"
#include "inc/dtof_api.h"

#define DTOF_SENSOR_DATA_LENGTH 38
typedef struct
{
    dtof_uint8_t uuid[DTOF_UUID_LENGTH];
    dtof_uint16_t sensor_data[DTOF_SENSOR_DATA_LENGTH];
} sensor_database_t;

sensor_database_t sensor_database[] =
{
    {
        .uuid = {83, 80, 65, 67, 69, 88, 95, 48, 48, 48, 48, 53, 255, 255, 255, 255},
        .sensor_data = {0, 0, 0, 0, 0, 0, 0, 1, 3, 14, 24, 28, 23, 17, 15, 13, 12, 11, 10, 9, 9, 8, 8, 8, 9, 8, 8, 7, 7, 6, 6, 5, 1, 0, 1197, 133, 254, 42,},
    },
    {
        .uuid = {78, 54, 49, 85, 48, 54, 255, 255, 255, 12, 22, 143, 255, 255, 255, 255},
        .sensor_data = {0, 0, 0, 0, 0, 0, 0, 1, 3, 14, 24, 28, 23, 17, 15, 13, 12, 11, 10, 9, 9, 8, 8, 8, 9, 8, 8, 7, 7, 6, 6, 5, 1, 0, 1197, 155, 254, 44,},
    },
};
