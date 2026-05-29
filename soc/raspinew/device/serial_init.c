#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <termios.h>
#include <errno.h>
#include <stdatomic.h>
#include <stdarg.h>


#include "device.h"
#include "serial_init.h"

int fd; // 串口文件描述符
volatile sig_atomic_t data_ready = 0; // 标志位

void sigio_handler(int signo) {
    if (signo == SIGIO) {
        data_ready = 1; // 只设置标志
    }
}


// 提供接口：获取并清除标志
int get_and_clear_data_ready() {
    sigset_t oldmask, blockmask;
    int value;

    // 阻塞 SIGIO，防止中断修改 data_ready
    sigemptyset(&blockmask);
    sigaddset(&blockmask, SIGIO);
    sigprocmask(SIG_BLOCK, &blockmask, &oldmask);

    // 临界区：读 + 清零
    value = data_ready;
    data_ready = 0;

    // 恢复信号屏蔽字
    sigprocmask(SIG_SETMASK, &oldmask, NULL);

    return value;
}

// 配置串口参数
int rpi_serial_init(const char *port_name) {
    
    struct termios tty;
    // 通过文件系统链接串口
    int serial_hd = open(port_name, O_RDWR | O_NOCTTY | O_NONBLOCK);
    
    if (serial_hd < 0) {
        perror("Error opening serial port");
        return -1;
    }

    // 配置串口
    tcgetattr(serial_hd, &tty);
    cfmakeraw(&tty);
    cfsetispeed(&tty, B115200);
    cfsetospeed(&tty, B115200);
    tcsetattr(serial_hd, TCSANOW, &tty);

    // 设置信号处理函数
    struct sigaction sa;
    sa.sa_handler = sigio_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGIO, &sa, NULL) < 0) {
        perror("sigaction");
        return 1;
    }

    // 设置当前进程为 fd 的“所有者”
    fcntl(serial_hd, F_SETOWN, getpid());

    int flags = fcntl(serial_hd, F_GETFL);
    fcntl(serial_hd, F_SETFL, flags | O_ASYNC | O_NONBLOCK);
    
    #ifdef SERIAL_OLD
    // 应用配置
    if (tcsetattr(serial_hd, TCSANOW, &options) != 0) {
        perror("tcsetattr");
        return -1;
    }
    #endif
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
int rpi_serial_receive(const int serial_hd, char *buffer, const size_t buf_size) {
    int total_size = 0;
    int n = read(serial_hd, buffer, buf_size);
    
    if (n > 0) {
        while (n >= 1) {
            if (buffer[n - 1] == '\n' || buffer[n - 1] == '\r') {
                --n;
                buffer[n] = '\0';
            }
            else {
                break;
            }
        }
        total_size = n;
        buffer[n] = '\0';
        return total_size;
        // printf("收到数据: %s, size %d\n", buffer, total_size);
    } else if (n < 0 && errno != EAGAIN) {
        perror("read error");
    }
    
    return 0;
}


device_driver_ops_t device_uart_driver_ops = {
    .init = rpi_serial_init,
    .deinit = stm32_uart_deinit,
    .write = stm32_uart_write,
    .read = stm32_uart_read,
};