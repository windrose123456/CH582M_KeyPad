#ifndef __DEBUG_UTILS_H__
#define __DEBUG_UTILS_H__

#include "CONFIG.h"

/**
 * @brief  Convert byte array to hex ASCII string with space separators.
 * @param  data     Input byte array.
 * @param  len      Number of bytes.
 * @param  out_buf  Output buffer (must be at least len*3 bytes).
 * @param  out_len  Output: string length (excluding '\0').
 */
void hex_to_ascii(const uint8_t *data, uint16_t len, char *out_buf, uint16_t *out_len);

/**
 * @brief  Dump byte array as hex string via UART0.
 * @param  data   Input byte array.
 * @param  len    Number of bytes.
 * @param  prefix Optional prefix string (e.g. "TX: "), or NULL.
 */
void debug_hex_dump(const uint8_t *data, uint16_t len, const char *prefix);

#endif /* __DEBUG_UTILS_H__ */