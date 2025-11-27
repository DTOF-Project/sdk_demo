#include <signal.h>
#include <stdio.h>
#include <stdatomic.h>
#include "signal_manager.h"

static atomic_int keep_running = ATOMIC_VAR_INIT(1);

static void signal_handler(int sig) {
    printf("接收到信号 %d，准备退出...\n", sig);
    atomic_store(&keep_running, 0);
}

void signal_manager_init(void) {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    // 注册 SIGTSTP 信号的处理函数
    if (signal(SIGTSTP, signal_handler) == SIG_ERR) {
        perror("signal");
    }
}

int signal_manager_should_continue(void) {
    return atomic_load(&keep_running);
}

void signal_manager_notify_exit(void) {
    atomic_store(&keep_running, 0);
}