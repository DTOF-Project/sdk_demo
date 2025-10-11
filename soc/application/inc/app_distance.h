#ifndef _APP_DISTANCE_H_
#define _APP_DISTANCE_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    DISTANCE_NORMAL_MODE = 0,
    DISTANCE_DEBUG_MODE,
    DISTANCE_TEST_MODE,
    DISTANCE_UNKNOWN_MODE
} distance_mode_t;

#define DISTANCE_TEST_MODE_FRAME_NUM 150

void app_distance_process(void);

void app_set_distance_mode(int mode);
int app_get_distance_mode(void);

DTOF_RET dtof_enable_distance_debug_mode(void);

#ifdef __cplusplus
}
#endif

#endif // _APP_DISTANCE_H_