#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "cJSON.h"
#include <openssl/bio.h>
#include <openssl/evp.h>
#include "command_receiver.h"
#include "signal_manager.h"
#include "app_rapi/cmd/inc/app_cmd.h"

static pthread_t receiver_thread;
static char rx_buffer_in[BUFFER_SIZE];
static int sockfd;
static struct sockaddr_in server_addr, client_addr;

struct sockaddr_in* get_server_addr(void){
    return &server_addr;
}

struct sockaddr_in* get_client_addr(void){
    return &client_addr;
}

int get_sockfd(void){
    return sockfd;
}

// static char tx_buffer_out[BUFFER_SIZE];
// 添加响应发送函数
// static void send_response(int sockfd, struct sockaddr_in *client_addr,
//                          appc_ret_enum_t result, const char *message) {
//     cJSON *response = cJSON_CreateObject();
//     cJSON_AddNumberToObject(response, "status", result);
//     cJSON_AddStringToObject(response, "message", message ? message : "");

//     char *response_str = cJSON_PrintUnformatted(response);
//     if (response_str) {
//         sendto(sockfd, response_str, strlen(response_str), 0,
//                (struct sockaddr*)client_addr, sizeof(*client_addr));
//         free(response_str);
//     }
//     cJSON_Delete(response);
// }
void send_response(int sockfd, struct sockaddr_in *client_addr,
                         appc_ret_enum_t result, const char *message) {
    cJSON *response = (cJSON *)message;
    // cJSON_AddNumberToObject(response, "status", result);
    // cJSON_AddStringToObject(response, "message", message ? message : "");

    char *response_str = cJSON_PrintUnformatted(response);
    if (response_str) {
        sendto(sockfd, response_str, strlen(response_str), 0,
                (struct sockaddr*)client_addr, sizeof(*client_addr));
        free(response_str);
    }
    // cJSON_Delete(response);
}

void debug_print_client_addr(const struct sockaddr_in *client_addr) {
    if (client_addr == NULL) {
        printf("客户端地址为空\n");
        return;
    }

    char client_ip[INET_ADDRSTRLEN];
    if (inet_ntop(AF_INET, &(client_addr->sin_addr), client_ip, INET_ADDRSTRLEN) == NULL) {
        perror("无法将 IP 地址转换为字符串");
        return;
    }

    unsigned short client_port = ntohs(client_addr->sin_port);
    printf("客户端 IP 地址: %s\n", client_ip);
    printf("客户端端口号: %hu\n", client_port);
}

/**
 * @brief 将 uint16_t 数组转换为十六进制字符串
 * @param hex_str 输入uint16_t 数组(如 {0x1234, 0x4567})
 * @param length 输入参数, 数组长度(元素个数)
 * @return uint16_t* 分配的数组指针，需由调用者释放；失败返回 NULL
 */
char* uint16_array_to_hex_string(const uint16_t *data, uint16_t length) {
    if (!data || length == 0) return NULL;

    char *hex_str = malloc(length * 4 + 1);
    if (!hex_str) return NULL;

    char *ptr = hex_str;  // 移动指针代替索引
    for (uint16_t i = 0; i < length; i++) {
        ptr += snprintf(ptr, 5, "%04X", data[i]);  // 直接写入到指针位置
    }

    return hex_str;
}

/**
 * @brief 将十六进制字符串转换为 uint16_t 数组
 * @param hex_str 输入的十六进制字符串 (如 "A1B2C3D4")
 * @param length 输出参数，返回数组长度(元素个数)
 * @return uint16_t* 分配的数组指针，需由调用者释放；失败返回 NULL
 */
uint16_t* hex_string_to_uint16_array(const char *hex_str, uint16_t *length) {
    if (!hex_str || !length) return NULL;

    size_t hex_len = strlen(hex_str);
    if (hex_len == 0 || hex_len % 4 != 0) {  // 每个 uint16_t 需要 4 字符
        return NULL;
    }

    *length = hex_len / 4;
    uint16_t *array = malloc(*length * sizeof(uint16_t));
    if (!array) return NULL;

    for (uint16_t i = 0; i < *length; i++) {
        char hex_part[5] = {0};
        strncpy(hex_part, hex_str + i * 4, 4);  // 提取 4 字符（如 "A1B2"）

        // 检查是否为有效十六进制
        for (int j = 0; j < 4; j++) {
            if (!isxdigit(hex_part[j])) {
                free(array);
                return NULL;
            }
        }

        array[i] = (uint16_t)strtoul(hex_part, NULL, 16);  // 转换为 uint16_t
    }
    return array;
}

// Base64 解码函数
int base64_decode(const char *base64_input, uint8_t **decoded_output, int *decoded_length) {
    BIO *bio, *b64;
    int input_length = strlen(base64_input);
    *decoded_output = (uint8_t *)malloc(input_length);
    printf("base64_decode%d\n",__LINE__);
    bio = BIO_new_mem_buf(base64_input, -1);
    printf("base64_decode%d\n",__LINE__);
    b64 = BIO_new(BIO_f_base64());
    printf("base64_decode%d\n",__LINE__);
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL); // 避免换行符影响
    printf("base64_decode%d\n",__LINE__);
    bio = BIO_push(b64, bio);
    printf("base64_decode%d\n",__LINE__);

    *decoded_length = BIO_read(bio, *decoded_output, input_length);
    printf("base64_decode%d\n",__LINE__);
    BIO_free_all(bio);

    return (*decoded_length > 0) ? 0 : -1;
}

int base64_encode(const uint8_t *input, int input_length, char **encoded_output) {
    BIO *bio, *b64;
    char *buffer;
    long buffer_len;

    b64 = BIO_new(BIO_f_base64());
    if (!b64) return -1;

    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);  // 禁用换行符

    bio = BIO_new(BIO_s_mem());
    if (!bio) {
        BIO_free(b64);
        return -1;
    }

    bio = BIO_push(b64, bio);

    // 写入数据并刷新
    if (BIO_write(bio, input, input_length) != input_length) {
        BIO_free_all(bio);
        return -1;
    }
    BIO_flush(bio);

    // 获取内存 BIO 中的数据指针和长度（关键修改点）
    buffer_len = BIO_get_mem_data(bio, &buffer);
    if (buffer_len <= 0) {
        BIO_free_all(bio);
        return -1;
    }

    // 分配输出缓冲区（包括终止符）
    *encoded_output = (char *)malloc(buffer_len + 1);
    if (!*encoded_output) {
        BIO_free_all(bio);
        return -1;
    }

    memcpy(*encoded_output, buffer, buffer_len);
    (*encoded_output)[buffer_len] = '\0';  // 添加终止符

    BIO_free_all(bio);
    return 0;
}

// JSON命令解析函数
static appc_ret_enum_t parse_json_command(dtof_uint8_t * json_str, uint16_t json_len,const char* out_put,uint16_t *json_len_out_put)
{
    if(!json_str){
        DTOF_LOG("输入 JSON 字符串为空\n");
        return APPC_PROTO_PARSE_PROCESS_ERROR;
    }

    cJSON *rx_jason_root = cJSON_Parse((const char *)json_str);

    uint16_t job_id = 0;
    if (!rx_jason_root) {
        DTOF_LOG("JSON解析失败\n");
        return APPC_PROTO_PARSE_PROCESS_ERROR;
    }
    printf("解码后数据：%d\n",__LINE__);
    // 解析命令ID
    cJSON *cmd_obj = cJSON_GetObjectItem(rx_jason_root, "cmd");
    if (!cmd_obj || !cJSON_IsNumber(cmd_obj)) {
        DTOF_LOG("无效的命令格式\n");
        cJSON_Delete(rx_jason_root);
        return APPC_PROTO_PARSE_PROCESS_ERROR;
    }
    uint8_t cmd_id = (uint8_t)cmd_obj->valueint;
    printf("解码后数据：%d\n",__LINE__);
    // 解析任务ID（可选）

    cJSON *job_obj = cJSON_GetObjectItem(rx_jason_root, "job_id");
    if (job_obj && cJSON_IsNumber(job_obj)) {
        job_id = (uint8_t)job_obj->valueint;
    }
    printf("解码后数据：%d\n",__LINE__);

    appc_ret_enum_t ret = app_cmd_parsing(cmd_id, job_id, (dtof_uint8_t*)rx_jason_root, json_len,(dtof_uint8_t *)out_put,json_len_out_put);

    printf("解码后数据：%d\n",__LINE__);
    cJSON_Delete(rx_jason_root);
    return ret;
}

static void* command_receiver_thread(void* arg) {
    struct sched_param param;
    param.sched_priority = 10;  // 低优先级
    if (pthread_setschedparam(pthread_self(), SCHED_FIFO, &param) != 0) {
        perror("无法设置命令接收器线程优先级");
    }

    socklen_t addr_len = sizeof(client_addr);

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("命令接收器套接字创建失败");
        return NULL;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(UDP_PORT);

    if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("命令接收器端口绑定失败");
        close(sockfd);
        return NULL;
    }

    printf("命令接收器启动，监听端口 %d...\n", UDP_PORT);

    while (signal_manager_should_continue()) {
        fd_set readfds;
        struct timeval timeout;
        int retval;
        uint16_t output_len = 0;
        //memset(rx_buffer_in, 0, BUFFER_SIZE);
        // 设置 select() 的文件描述符集合
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);
        #define RECV_TIMEOUT_SEC 5
        // 设置超时时间
        // 设置超时时间为 50 毫秒
        timeout.tv_sec = 0;
        timeout.tv_usec = 100 * 1000; // 50 毫秒转换为微秒
        // 使用 select() 等待 socket 可读或超时
        retval = select(sockfd + 1, &readfds, NULL, NULL, &timeout);
        if (retval == -1) {
            printf("11111\n");
        } else if (retval == 0) {
        } else {
            // socket 可读，调用 recvfrom
            int recv_len = recvfrom(sockfd, rx_buffer_in, BUFFER_SIZE, 0,
                (struct sockaddr*)&client_addr, &addr_len);

            if (recv_len > 0) {
                rx_buffer_in[recv_len] = '\0';
                printf("收到命令: %s, 长度%d\n", rx_buffer_in, recv_len);

                cJSON *tx_json_root = cJSON_CreateObject();
                appc_ret_enum_t result;
                if (rx_buffer_in[0] == '{') {
                printf("JSON格式命令: %s\n", rx_buffer_in);
                    result = parse_json_command((dtof_uint8_t*)rx_buffer_in, (uint16_t)recv_len, (const char*)tx_json_root, &output_len);
                } else {
                    result = APPC_PROTO_PARSE_PROCESS_ERROR;
                }

                send_response(sockfd, &client_addr, result, (const char*)tx_json_root);
                cJSON_Delete(tx_json_root);
            } else if (recv_len == -1) {
                perror("recvfrom");
            }
        }
    }
    close(sockfd);
    printf("fredlei  close the command rec thread");
    return NULL;
}

int command_receiver_start(void) {
    if (pthread_create(&receiver_thread, NULL, command_receiver_thread, NULL) != 0) {
        perror("命令接收器线程创建失败");
        return -1;
    }
    return 0;
}

void command_receiver_wait(void) {
    pthread_join(receiver_thread, NULL);
}