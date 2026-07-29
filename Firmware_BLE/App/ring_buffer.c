#include "ring_buffer.h"
#include <string.h>

void ring_buffer_init(ring_buffer_t *rb, uint8_t *buf, uint16_t size) {
    rb->buffer = buf;
    rb->size = size;
    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;
}

void ring_buffer_reset(ring_buffer_t *rb) {
    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;
}

bool ring_buffer_push(ring_buffer_t *rb, uint8_t data) {
    if (rb->count >= rb->size) {
        return false;   // 已满
    }
    rb->buffer[rb->head] = data;
    rb->head = (rb->head + 1) % rb->size;
    rb->count++;
    return true;
}

bool ring_buffer_pop(ring_buffer_t *rb, uint8_t *data) {
    if (rb->count == 0) {
        return false;   // 为空
    }
    *data = rb->buffer[rb->tail];
    rb->tail = (rb->tail + 1) % rb->size;
    rb->count--;
    return true;
}

bool ring_buffer_peek(const ring_buffer_t *rb, uint8_t *data) {
    if (rb->count == 0) {
        return false;
    }
    *data = rb->buffer[rb->tail];
    return true;
}

bool ring_buffer_is_empty(const ring_buffer_t *rb) {
    return (rb->count == 0);
}

bool ring_buffer_is_full(const ring_buffer_t *rb) {
    return (rb->count >= rb->size);
}

uint16_t ring_buffer_available(const ring_buffer_t *rb) {
    return rb->count;
}

uint16_t ring_buffer_free_space(const ring_buffer_t *rb) {
    return rb->size - rb->count;
}

uint16_t ring_buffer_push_multiple(ring_buffer_t *rb, const uint8_t *data, uint16_t len) {
    uint16_t i;
    for (i = 0; i < len; i++) {
        if (!ring_buffer_push(rb, data[i])) {
            break;  // 缓冲区满，停止写入
        }
    }
    return i;  // 返回实际写入个数
}

uint16_t ring_buffer_pop_multiple(ring_buffer_t *rb, uint8_t *data, uint16_t len) {
    uint16_t i;
    for (i = 0; i < len; i++) {
        if (!ring_buffer_pop(rb, &data[i])) {
            break;
        }
    }
    return i;
}