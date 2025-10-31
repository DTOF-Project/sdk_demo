# SDK使用方法

1. 工程中添加sdk源文件
    1.1 dtof_api.c
    1.2 dtof_driver.c
    1.3 dtof_endian.c
    1.4 dtof_fix16_float.c
2. 注册dtof_driver.c中的函数
    2.1 DTOF_RET dtof_reg_burst_read(dtof_uint8_t reg_addr, dtof_uint16_t *reg_data_p, dtof_uint16_t len)
        burst读寄存器
    2.2 DTOF_RET dtof_reg_burst_write(dtof_uint8_t reg_addr, dtof_uint16_t *reg_data_p, dtof_uint16_t len)
        burst写寄存器
    2.3 void dtof_set_interrupt_flag(dtof_bool_t flag)
        设置中断标志位
    2.4 dtof_bool_t dtof_get_interrupt_flag(void)
        获取中断标志位
    2.5 void dtof_sleep_ms(dtof_uint32_t time)
        ms延时
3. dtof初始化
    调用dtof_init函数
4. 开始/停止测距
    4.1 开始
    4.2 停止
5. 获取测距结果
    调用dtof_get_fifo函数, 可以使用step2中注册的获取中断函数, 如：
    dtof_distance_result_t distance_result;
    if(dtof_get_interrupt_flag() == DTOF_TRUE){
        dtof_get_distance_result(&ack_p->distance_result);
        dtof_set_interrupt_flag(DTOF_FALSE);
    }

