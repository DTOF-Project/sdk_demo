/**
 * @file dtof_ringbuffer.h
 * @brief DTOF环形缓冲区实现
 * @author liuzihao
 * @date 2024/9/11
 */

#ifndef _DTOF_RINGBUFFER_H_
#define _DTOF_RINGBUFFER_H_

#include <stdint.h>
#include <stdbool.h>
#include "inc/dtof_base_type.h"

#ifdef __cplusplus
extern "C" {
#endif

// 缓冲区配置
#define DTOF_RING_BUFFER_SIZE    1024    // 必须是2的幂
#define DTOF_RING_BUFFER_MASK    (DTOF_RING_BUFFER_SIZE - 1)

// 类型定义
typedef uint16_t dtof_ring_size_t;

// 环形缓冲区结构
typedef struct {
    volatile uint8_t buffer[DTOF_RING_BUFFER_SIZE];  // 缓冲区数据
    volatile dtof_ring_size_t head;                  // 写入位置
    volatile dtof_ring_size_t tail;                  // 读取位置
    volatile bool is_full;                           // 满状态标志
} dtof_ring_buffer_t;

// 初始化函数
void dtof_ring_init(dtof_ring_buffer_t *rb);

// 数据操作函数
bool dtof_ring_push(dtof_ring_buffer_t *rb, uint8_t data);
bool dtof_ring_push_multi(dtof_ring_buffer_t *rb, const uint8_t *data, dtof_ring_size_t len);
bool dtof_ring_pop(dtof_ring_buffer_t *rb, uint8_t *data);
dtof_ring_size_t dtof_ring_pop_multi(dtof_ring_buffer_t *rb, uint8_t *data, dtof_ring_size_t len);

// 状态查询函数
static inline bool dtof_ring_is_empty(const dtof_ring_buffer_t *rb) {
    return (!rb->is_full && (rb->head == rb->tail));
}

static inline bool dtof_ring_is_full(const dtof_ring_buffer_t *rb) {
    return rb->is_full;
}

static inline dtof_ring_size_t dtof_ring_count(const dtof_ring_buffer_t *rb) {
    dtof_ring_size_t count = DTOF_RING_BUFFER_SIZE;
    if (!rb->is_full) {
        count = (rb->head - rb->tail) & DTOF_RING_BUFFER_MASK;
    }
    return count;
}

static inline dtof_ring_size_t dtof_ring_free_space(const dtof_ring_buffer_t *rb) {
    return DTOF_RING_BUFFER_SIZE - dtof_ring_count(rb);
}

#ifdef __cplusplus
}
#endif

#endif // _DTOF_RINGBUFFER_H_
