#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "serial_init.h"

// 配置串口参数
int rpi_serial_init(const char *port_name) {
    // 通过文件系统链接串口
    int serial_hd = open(port_name, O_RDWR | O_NOCTTY | O_NDELAY);
    
    if (serial_hd < 0) {
        perror("Error opening serial port");
        return -1;
    }
    
    // 配置串口
    struct termios options;
    
    // 获取当前配置
    if (tcgetattr(serial_hd, &options) != 0) {
        perror("tcgetattr");
        return -1;
    }
    
    // 设置波特率：115200
    cfsetispeed(&options, B115200);
    cfsetospeed(&options, B115200);
    
    // 配置数据位：8位
    options.c_cflag &= ~CSIZE;  // 清空数据位设置
    options.c_cflag |= CS8;      // 8位数据
    
    // 配置奇偶校验：无校验
    options.c_cflag &= ~PARENB; 
    
    // 配置停止位：1位
    options.c_cflag &= ~CSTOPB;
    
    // 禁用硬件流控
    options.c_cflag &= ~CRTSCTS;
    
    // 启用接收 & 本地模式
    options.c_cflag |= (CLOCAL | CREAD);
    
    // 原始输入模式 (禁用规范模式)
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    
    // 原始输出模式
    options.c_oflag &= ~OPOST;
    
    // 设置读取超时
    options.c_cc[VMIN] = 0;     // 最小读取字节数
    options.c_cc[VTIME] = 1;   // 等待时间（0.1秒单位）可以在select的timeout参数中设置
    // VMIN=0, VTIME>0：定时返回（即使无数据）
    // VMIN>0, VTIME>0：至少读取VMIN字节或超时
    // VMIN>0, VTIME=0：阻塞直到读取VMIN字节
    
    // 应用配置
    if (tcsetattr(serial_hd, TCSANOW, &options) != 0) {
        perror("tcsetattr");
        return -1;
    }
    
    return serial_hd;
}

// 串口发送函数
int rpi_serial_send(const int serial_hd, const char *data, const size_t len) {
    ssize_t written = write(serial_hd, data, len);
    if (written < 0) {
        perror("Write failed");
        return -1;
    }
    return written;
}

// 串口 print
int rpi_serial_printf(int serial_hd, const char *fmt, ...) {
    char buffer[512];  // 缓冲区大小可按需调整
    va_list args;
    va_start(args, fmt);
    int n = vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    if (n < 0) {
        return -1;  // 格式化失败
    }
    if ((size_t)n >= sizeof(buffer)) {
        // 输出被截断，可以考虑动态分配更大 buffer
        n = sizeof(buffer) - 1;
    }

    return rpi_serial_send(serial_hd, buffer, n);
}

// 串口接收函数
// 带超时和缓冲区的串口接收
int rpi_serial_receive(const int serial_hd, char *buffer, const size_t buf_size) {
    size_t total_received = 0;
    struct timeval timeout = {0, 100}; // 等待0.02秒
    fd_set read_fds;
    
    FD_ZERO(&read_fds);
    FD_SET(serial_hd, &read_fds);
    
    // 等待数据可用
    int ret = select(serial_hd + 1, &read_fds, NULL, NULL, &timeout);
    
    if (ret < 0) {
        perror("select failed");
        return -1;
    }
    
    if (ret == 0) {
        return 0; // 超时，无数据
    }
    
    // 确定有数据可读
    if (FD_ISSET(serial_hd, &read_fds)) {
        ssize_t n = read(serial_hd, buffer, buf_size - 1);
        if (n < 0) {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                perror("read error");
                return -1;
            }
            return 0;
        }
        
        if (n == 0) {
            return 0; // 对端关闭
        }

        // 删除末尾的换行符
        while (n >= 1) {
            if (buffer[n - 1] == '\n' || buffer[n - 1] == '\r') {
                --n;
                buffer[n] = '\0';
            }
            else {
                break;
            }
        }
        
        total_received = n;
        buffer[n] = '\0';
        return total_received;
    }
    
    return 0;
}
