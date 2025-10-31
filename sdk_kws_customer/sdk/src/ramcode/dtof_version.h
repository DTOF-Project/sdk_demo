#include "src/ramcode/l3_pre_config.ram"
#include "src/ramcode/l3_distance_init.ram"
#include "src/ramcode/l3_distance_mode.ram"
#include "src/ramcode/a05_distance_init.ram"
#include "src/ramcode/a05_distance_mode.ram"

#define DTOF_VERSION_UNKNOWN 0xFFFF
#define DTOF_VERSION_MAJOR 3
#define DTOF_VERSION_MINOR 2
#define DTOF_VERSION_PATCH 7

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#define DTOF_VERSION_STRING STR(DTOF_VERSION_MAJOR) "." STR(DTOF_VERSION_MINOR) "." STR(DTOF_VERSION_PATCH)

typedef enum
{
    DTOF_L3_PRE_CONFIG = 0,
    DTOF_L3_DISTANCE_INIT,
    DTOF_L3_DISTANCE_MODE,
    DTOF_A05_DISTANCE_INIT,
    DTOF_A05_DISTANCE_MODE
} dtof_ram_code_type_t;

typedef struct {
    const dtof_uint16_t* ram_code_ptr;
    dtof_uint16_t  ram_code_size;
} dtof_ram_code_info_t;

dtof_ram_code_info_t ram_code_all[] = {
    { l3_pre_config, sizeof(l3_pre_config) / sizeof(dtof_uint16_t) },
    { l3_distance_init, sizeof(l3_distance_init) / sizeof(dtof_uint16_t) },
    { l3_distance_mode, sizeof(l3_distance_mode) / sizeof(dtof_uint16_t) },
    { a05_distance_init, sizeof(a05_distance_init) / sizeof(dtof_uint16_t) },
    { a05_distance_mode, sizeof(a05_distance_mode) / sizeof(dtof_uint16_t) },
};
