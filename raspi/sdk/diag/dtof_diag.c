#include "inc/dtof_api.h"
#include "inc/dtof_log.h"
#include "inc/dtof_common.h"
#include "inc/dtof_driver.h"

void dtof_diag_test(dtof_uint8_t device_id)
{
    dtof_uint16_t chip_id;
    int32_t frame_cnt = 0;
    dtof_bool_t is_new_flag;
    dtof_int32_t read_distance_offset;
    dtof_uint16_t xtalk_data_read[XTALK_DATA_SIZE];
    dtof_distance_result_t distance_result;
    char c;
    printf("please input cmd: ");
    c = uart_getchar();
    printf("receive cmd: %c\n", c);

    switch (c)
    {
    case '1':
        /* 测试i2c 通讯 */
        printf("Testing I2C communicatio OK...\n");
        break;
    case '2':
        // 测试测距数据获取
        printf("Testing frame start and stop is ok \n");
        // start 后 读取 frameid 是不是增加的 然后停止
        break;
    case '3':
        // 600mm 校准测试
        DTOF_CHECK_RET_VOID(dtof_init_and_wait_for_ready(device_id, &chip_id, DO_XTALK_CALIBRATION_MODE), "dtof init and wait for ready failed\n");
        printf("Testing xtalk calibration is ok \n");
        break;
    case '4':
        // 600mm 校准测试 flash中获取
        dtof_get_xtalk_data_from_flash(device_id, xtalk_data_read);
        printf("xtalk data = ");
        for (int i = 0; i < XTALK_DATA_SIZE; i++)
        {
            printf("%d, ", xtalk_data_read[i]);
        }
        printf("\n");
        break;
    case '5':
        //200mm校准测试
        DTOF_CHECK_RET_VOID(dtof_init_and_wait_for_ready(device_id, &chip_id, DO_OFFSET_CALIBRATION_MODE), "dtof init and wait for ready failed\n");
        printf("Testing distance calibration is ok \n");
        break;
    case '6':
        // 200mm 校准数据从flash中获取
        dtof_get_distance_offset_from_flash(device_id, &read_distance_offset);
        printf("distance offset = %d\n", read_distance_offset);
        break;
    case '7':
        // 定点数据测试
        DTOF_CHECK_RET_VOID(dtof_init_and_wait_for_ready(device_id, &chip_id, NORMAL_DISTANCE_MODE), "dtof init and wait for ready failed\n");
        DTOF_CHECK_RET_VOID(dtof_start_distance_measure(device_id), "dtof start distance measure failed\n");

        while(1)
        {
            dtof_get_distance_result(device_id, DO_OFFSET_CALIBRATION_MODE, &distance_result, &is_new_flag);
            if (is_new_flag == DTOF_TRUE)
            {
                frame_cnt++;
                if (frame_cnt < 50)
                {
                    continue;
                }
                if (frame_cnt == 200)
                {
                    frame_cnt = 0;
                    dtof_stop_distance_measure(device_id);
                    return;
                }
                printf("%d, %d, %d, %d, %.6f, %d\n",
                   distance_result.frame_id, distance_result.first_target, distance_result.first_intensity, distance_result.main_nflash, distance_result.ambient, distance_result.is_legal_frame);
            }
        }

        break;
    default:
        break;
    }
}