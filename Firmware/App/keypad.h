#ifndef __KEYPAD_H__
#define __KEYPAD_H__

#include "CH58x_common.h"

// 按键引脚定义（PB1）
#define KEY_PIN         GPIO_Pin_1
#define KEY_PORT        GPIOB
#define KEY_GPIO_CLK    RCC_GPIOB

// 按键按下/释放的电平（根据你的硬件电路）
// 如果你的按键按下时PB1为低电平（常见上拉接法），则：
#define KEY_PRESSED     0
#define KEY_RELEASED    1

void KeyPad_Init(void);
uint8_t KeyPad_Scan(void);
uint8_t KeyPad_GetState(uint8_t key_index);
uint16_t KeyPad_GetBitmap(void);

#endif