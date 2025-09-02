#ifndef SIGNAL_MANAGER_H
#define SIGNAL_MANAGER_H

// 初始化信号管理器
void signal_manager_init(void);

// 检查程序是否应该继续运行
int signal_manager_should_continue(void);

// 通知程序退出
void signal_manager_notify_exit(void);

#endif // SIGNAL_MANAGER_H