#ifndef _APP_CMD_H_
#define _APP_CMD_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*cmd_handler_t)(const char *cmd);

typedef struct {
    const char *name;       // 命令字符串 (可为前缀)
    int is_prefix;          // 是否是前缀命令 (1 = 前缀匹配, 0 = 完全匹配)
    cmd_handler_t handler;  // 处理函数
} cmd_entry_t;

void parse_cmd_process(char byte);

#ifdef __cplusplus
}
#endif

#endif // _APP_CMD_H_