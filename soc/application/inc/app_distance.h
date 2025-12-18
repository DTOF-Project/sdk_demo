#ifndef _APP_DISTANCE_H_
#define _APP_DISTANCE_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    DISTANCE_NORMAL_MODE = 0, // 30Hz 测距, 输出api结果
    DISTANCE_DEBUG_MODE,      // debug模式, 不保证帧率, 输出reg, hist, fifo数据
    DISTANCE_TEST_MODE,       // test模式, 在debug模式的基础上在指定帧数后自动停止
    DISTANCE_UNKNOWN_MODE
} distance_mode_t;

#define DISTANCE_TEST_MODE_FRAME_NUM 150

void app_distance_process(void);

void app_set_distance_mode(int mode);
int app_get_distance_mode(void);

DTOF_RET dtof_trigger_next_frame(void);

#ifdef __cplusplus
}
#endif

#endif // _APP_DISTANCE_H_