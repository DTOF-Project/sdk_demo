#include <pthread.h>
#include "dtof_interrupt_handler.h"
#include <string.h>
#include <errno.h>
#include <sys/time.h> // For gettimeofday
#include <time.h>     // For struct timespec
#define QUEUE_SIZE 32

static dtof_raw_data_t data_queue[QUEUE_SIZE];
static int queue_head = 0;
static int queue_tail = 0;
static int queue_count = 0;
static pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t queue_cond = PTHREAD_COND_INITIALIZER;

void dtof_queue_init(void) {
    pthread_mutex_init(&queue_mutex, NULL);
    pthread_cond_init(&queue_cond, NULL);
}

void dtof_queue_deinit(void) {
    pthread_mutex_destroy(&queue_mutex);
    pthread_cond_destroy(&queue_cond);
}

int dtof_queue_push(dtof_raw_data_t *data) {
    pthread_mutex_lock(&queue_mutex);

    if (queue_count >= QUEUE_SIZE) {
        pthread_mutex_unlock(&queue_mutex);
        return -1;  // 队列满
    }

    memcpy(&data_queue[queue_tail], data, sizeof(dtof_raw_data_t));
    queue_tail = (queue_tail + 1) % QUEUE_SIZE;
    queue_count++;

    pthread_cond_signal(&queue_cond);
    pthread_mutex_unlock(&queue_mutex);

    // printf("queue_count = %d\n", queue_count);
    return 0;
}

int dtof_queue_pop(dtof_raw_data_t *data) {
    pthread_mutex_lock(&queue_mutex);

    while (queue_count == 0) {
        pthread_cond_wait(&queue_cond, &queue_mutex);
    }

    memcpy(data, &data_queue[queue_head], sizeof(dtof_raw_data_t));
    queue_head = (queue_head + 1) % QUEUE_SIZE;
    queue_count--;

    pthread_mutex_unlock(&queue_mutex);

    // printf("dtof_queue_pop\n");
    return 0;
}



int dtof_queue_pop_timeout(dtof_raw_data_t *data, long timeout_ms) {
    pthread_mutex_lock(&queue_mutex);

    struct timespec timeout_time;
    struct timeval now;
    int ret = 0;

    // 获取当前时间
    gettimeofday(&now, NULL);

    // 计算超时时间
    long long nanoseconds = (long long)now.tv_sec * 1000000000 + (long long)now.tv_usec * 1000 + timeout_ms * 1000000;
    timeout_time.tv_sec = nanoseconds / 1000000000;
    timeout_time.tv_nsec = nanoseconds % 1000000000;

    while (queue_count == 0) {
        ret = pthread_cond_timedwait(&queue_cond, &queue_mutex, &timeout_time);
        if (ret == ETIMEDOUT) {
            pthread_mutex_unlock(&queue_mutex);
            return -1; // 超时返回错误
        } else if (ret != 0) {
            pthread_mutex_unlock(&queue_mutex);
            return -2; // 其他错误
        }
        // 如果被唤醒但队列仍然为空，则继续等待
    }

    memcpy(data, &data_queue[queue_head], sizeof(dtof_raw_data_t));
    queue_head = (queue_head + 1) % QUEUE_SIZE;
    queue_count--;

    pthread_mutex_unlock(&queue_mutex);

    return 0; // 成功弹出数据
}