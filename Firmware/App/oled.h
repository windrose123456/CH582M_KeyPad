#ifndef __OLED_H__
#define __OLED_H__

#include "CH58x_common.h"

#define OLED_WIDTH  128
#define OLED_HEIGHT 32

#define OLED_ADDRESS 0x3C  // 7位地址

// 若使用硬件I2C，请取消下面注释；若使用软件模拟I2C，请注释掉
//#define OLED_HARDWARE_I2C

void OLED_Init(void);
void OLED_Clear(void);
void OLED_Update(void);
void OLED_ShowChar(uint8_t x, uint8_t y, char ch, uint8_t width, uint8_t height);
void OLED_ShowString(uint8_t x, uint8_t y, const char *str, uint8_t width, uint8_t height);
void OLED_ShowNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t width, uint8_t height);

#endif