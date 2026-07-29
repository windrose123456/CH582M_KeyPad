#ifndef __RING_BUFFER_H
#define __RING_BUFFER_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief 环形缓冲区结构体
 * @note 用户只需提供存储数组和大小，初始化后即可使用
 */
typedef struct {
    uint8_t *buffer;     // 指向存储区的指针
    uint16_t size;       // 缓冲区总大小（字节数）
    uint16_t head;       // 写入索引
    uint16_t tail;       // 读取索引
    uint16_t count;      // 当前存储的数据字节数
} ring_buffer_t;

/**
 * @brief 初始化环形缓冲区
 * @param rb   指向环形缓冲区实例的指针
 * @param buf  用户提供的存储数组
 * @param size 数组大小（必须 > 0）
 */
void ring_buffer_init(ring_buffer_t *rb, uint8_t *buf, uint16_t size);

/**
 * @brief 重置缓冲区（清空数据，但不清除数组内容）
 * @param rb 指向实例的指针
 */
void ring_buffer_reset(ring_buffer_t *rb);

/**
 * @brief 向缓冲区写入一个字节
 * @param rb   实例指针
 * @param data 要写入的字节
 * @return true 成功写入，false 缓冲区已满
 */
bool ring_buffer_push(ring_buffer_t *rb, uint8_t data);

/**
 * @brief 从缓冲区读取一个字节
 * @param rb   实例指针
 * @param data 输出参数，存储读取的字节
 * @return true 成功读取，false 缓冲区为空
 */
bool ring_buffer_pop(ring_buffer_t *rb, uint8_t *data);

/**
 * @brief 查看缓冲区头部字节（不移除）
 * @param rb   实例指针
 * @param data 输出参数，存储查看的字节
 * @return true 成功，false 缓冲区为空
 */
bool ring_buffer_peek(const ring_buffer_t *rb, uint8_t *data);

/**
 * @brief 检查缓冲区是否为空
 * @param rb 实例指针
 * @return true 空，false 非空
 */
bool ring_buffer_is_empty(const ring_buffer_t *rb);

/**
 * @brief 检查缓冲区是否已满
 * @param rb 实例指针
 * @return true 满，false 未满
 */
bool ring_buffer_is_full(const ring_buffer_t *rb);

/**
 * @brief 获取当前已存储的字节数
 * @param rb 实例指针
 * @return 字节数
 */
uint16_t ring_buffer_available(const ring_buffer_t *rb);

/**
 * @brief 获取剩余可用空间（字节数）
 * @param rb 实例指针
 * @return 剩余空间
 */
uint16_t ring_buffer_free_space(const ring_buffer_t *rb);

/**
 * @brief 批量写入多个字节
 * @param rb    实例指针
 * @param data  数据源指针
 * @param len   要写入的字节数
 * @return 实际成功写入的字节数
 */
uint16_t ring_buffer_push_multiple(ring_buffer_t *rb, const uint8_t *data, uint16_t len);

/**
 * @brief 批量读取多个字节
 * @param rb    实例指针
 * @param data  目标缓冲区
 * @param len   期望读取的字节数
 * @return 实际读取的字节数
 */
uint16_t ring_buffer_pop_multiple(ring_buffer_t *rb, uint8_t *data, uint16_t len);

#endif