#ifndef COMMAND_RECEIVER_H
#define COMMAND_RECEIVER_H

#include "cmd/inc/app_cmd.h"

#define UDP_PORT 8888
#define BUFFER_SIZE 8096  // 增加缓冲区大小以支持更大的数据包

int command_receiver_start(void);
void command_receiver_wait(void);

struct sockaddr_in* get_server_addr(void);
struct sockaddr_in* get_client_addr(void);
int get_sockfd(void);
void send_response(int sockfd, struct sockaddr_in *client_addr,
    appc_ret_enum_t result, const char *message);

char* uint16_array_to_hex_string(const uint16_t *data, uint16_t length);
uint16_t* hex_string_to_uint16_array(const char *hex_str, uint16_t *length);

#endif // COMMAND_RECEIVER_H