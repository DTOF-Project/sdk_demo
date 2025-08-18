# dev为高权限目录, 用于功能测试, 不提供给客户

## api list

### customer: sdk/api
1. dtof_get_fifo                        获取测距结果
2. dtof_get_uuid                        获取uuid
3. dtof_set_mcu_status                  设置inner mcu状态(bypass/wakeup inner mcu after one frame)
4. dtof_get_error_info                  获取错误信息
5. dtof_clear_error_info                清除错误信息
6. dtof_clear_eye_safety_error_info     清除人眼安全错误信息
7. dtof_set_io_voltage                  设置io电压
8. dtof_ram_code_burn                   升级ram代码                                       未完成
9. dtof_read_reg_running                运行中读寄存器
10. dtof_write_reg_running              运行中写寄存器

### dev: sdk/dev/api
1.  dtof_bypass_inner_mcu_external       外部直接bypass inner mcu (仅提供接口, 一般不用)
2.  dtof_wakeup_inner_mcu_external       外部唤醒inner mcu
3.  dtof_get_distance_result             获取测距结果
4.  dtof_get_multiple_result             获取多点结果
5.  dtof_switch_mode                     切换运行模式(烟感/测距/多点)
6.  dtof_reset_and_close_wdt             复位并且关闭看门狗
7.  dtof_switch_infrared_mode            切换到红外模式
8.  dtof_start_send_infrared
9.  dtof_send_infrared_info
10. dtof_quit_infrared_mode              退出红外模式
11. dtof_switch_multiple_mode            切换运行模式到多点
12. dtof_switch_multiple_mode_another    切换运行模式到多点
13. dtof_manual_control                  屏蔽/运行帧
14. dtof_change_report_result_mode       设置inner mcu上报数据时中断触发模式
15. dtof_change_fsm_status               更改状态机状态
16. dtof_set_smoke_ignore_frame          设置烟感省略帧数
17. dtof_stop_accumulate_mode            停止累加模式
18. dtof_debug                           debug cjtag
19. dtof_get_somke_result                获取烟感结果

10.3 dtof_get_multiple_result
11. dtof_iic_calibration                iic累加模式校准, ram code
11.1 dtof_iic_calculate                 iic 校准计算
11.2 dtof_iic_calculate_ret             iic 校准结果