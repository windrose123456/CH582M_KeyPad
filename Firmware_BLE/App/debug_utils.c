#include "debug_utils.h"

/**
 * @brief 将二进制数据转换为十六进制 ASCII 字符串（空格分隔）
 * @param data     二进制数据指针
 * @param len      数据长度
 * @param out_buf  输出缓冲区（需足够大，至少 len*3 字节）
 * @param out_len  输出字符串长度（不含 '\0'）
 */
void hex_to_ascii(const uint8_t *data, uint16_t len, char *out_buf, uint16_t *out_len) {
    uint16_t idx = 0;
    for (uint16_t i = 0; i < len; i++) {
        // 每个字节转两个十六进制字符，占 2 字节
        sprintf(&out_buf[idx], "%02X", data[i]);
        idx += 2;
        // 字节之间加空格（最后一个不加）
        if (i < len - 1) {
            out_buf[idx++] = ' ';
        }
    }
    out_buf[idx] = '\0';   // 字符串结束符
    *out_len = idx;
}

/**
 * @brief 将二进制数据转换为十六进制 ASCII 字符串并通过 UART0 输出
 * @param data     二进制数据指针
 * @param len      数据长度
 * @param prefix   可选前缀字符串（例如 "TX: " 或 "RX: "），可为 NULL
 */
void debug_hex_dump(const uint8_t *data, uint16_t len, const char *prefix) {
    // 缓冲区大小：每个字节占 3 个字符（2个HEX + 1个空格），最后加 '\0'
    // 限制最大打印长度，防止缓冲区溢出
    #define DEBUG_MAX_LEN   128
    char debug_str[DEBUG_MAX_LEN * 3 + 1];
    uint16_t idx = 0;

    // 如果有前缀，先发送前缀
    if (prefix != NULL) {
        UART0_SendString((uint8_t *)prefix, strlen(prefix));
    }

    // 实际打印长度限制，防止缓冲区溢出
    uint16_t print_len = (len > DEBUG_MAX_LEN) ? DEBUG_MAX_LEN : len;

    // 转换每个字节
    for (uint16_t i = 0; i < print_len; i++) {
        // 每个字节转两个十六进制字符
        debug_str[idx++] = "0123456789ABCDEF"[(data[i] >> 4) & 0x0F];
        debug_str[idx++] = "0123456789ABCDEF"[data[i] & 0x0F];
        // 字节之间加空格（最后一个不加）
        if (i < print_len - 1) {
            debug_str[idx++] = ' ';
        }
    }
    debug_str[idx] = '\0';   // 字符串结束符

    // 通过 UART0 发送
    UART0_SendString((uint8_t *)debug_str, idx);

    // 如果数据被截断，加 "..."
    if (len > DEBUG_MAX_LEN) {
        UART0_SendString((uint8_t *)" ...", 4);
    }

    // 发送换行
    UART0_SendString((uint8_t *)"\r\n", 2);
}