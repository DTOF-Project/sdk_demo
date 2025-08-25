#ifndef SERIAL_INIT_H
#define SERIAL_INIT_H

// 串口设备
#define SERIAL_PORT "/dev/ttyGS0"  // 硬件串口

// 函数声明
/**
 * @brief 初始化串口通信
 * @param port_name 串口的设备路径
 * @return >0:串口handle <0:失败
 */
int rpi_serial_init(const char *port_name);
/**
 * @brief 向串口发送消息
 * @param serial_hd 串口handle
 * @param data 发送信息
 * @param len 发送信息长度
 * @return 0:成功 <0:失败
 */
int rpi_serial_send(const int serial_hd, const char *data, const size_t len);

/**
 * @brief 串口发送数据 类printf 格式
 * @param serial_hd 串口handle
 * @param fmt 参数信息
 */
int rpi_serial_printf(int serial_hd, const char *fmt, ...);

/**
 * @brief 从串口接收消息
 * @param serial_hd 串口handle
 * @param buffer 接收信息缓冲
 * @param buf_size 最长接收信息长度
 * @return >0:接收信息长度 0:无数据 <0:异常
 */
int rpi_serial_receive(const int serial_hd, char *buffer, const size_t buf_size);

#endif // SERIAL_INIT_H