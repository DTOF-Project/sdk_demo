#include <stdint.h>
#include "dtof_frame_control.h"
#include "inc/dtof_log.h"
#include "inc/dtof_api.h"
#include "inc/dev/dtof_reg.h"

static uint32_t g_dtof_mode_type = 0;
static uint32_t g_dtof_run_state = 0;

uint32_t get_dtof_mode_type(void){
    return g_dtof_mode_type;
}

void set_dtof_mode_type(uint32_t value){
    g_dtof_mode_type = value;
}

uint32_t get_dtof_run_state(void){
    return g_dtof_run_state;
}

extern DTOF_RET dtof_write_reg_running(dtof_uint16_t reg_addr, dtof_uint16_t reg_data);
// TODO:@liuzihao return judge
void set_dtof_run_state(uint32_t value){
    g_dtof_run_state = value;
    switch(value){
        case DTOF_FARME_START:
        {
            dtof_write_reg_running(DTOF_REG202, DTOF_START_FRAME_FLAG);
            break;
        }
        case DTOF_FARME_STOP:
        {
            dtof_write_reg_running(DTOF_REG202, DTOF_STOP_FRAME_FLAG);
            break;
        }
        case DTOF_FARME_QUIT:
        {
            dtof_write_reg_running(DTOF_REG202, DTOF_QUIT_FRAME_FLAG);
            break;
        }
        default:
        {
            DTOF_LOG_ERR("无效的状态");
            break;
        }
    }
}
