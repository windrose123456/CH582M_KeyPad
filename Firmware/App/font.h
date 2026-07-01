#ifndef __FONT_H__
#define __FONT_H__

#include <stdint.h>
#include "CH58x_common.h"

// 8x6 字体 (6列 x 8行)，每个字符6字节
extern const uint8_t FONT_8x6[95][6];

// 8x8 字体 (8列 x 8行)，每个字符8字节
extern const uint8_t FONT_8x8[95][8];

#endif