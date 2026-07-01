#ifndef __KEYPAD_H__
#define __KEYPAD_H__

#include "CH58x_common.h"

// 按键数量
#define KEY_NUM         15

// 15个按键的引脚定义（PB0~PB14）
#define KEY_1_PIN       GPIO_Pin_0
#define KEY_2_PIN       GPIO_Pin_1
#define KEY_3_PIN       GPIO_Pin_2
#define KEY_4_PIN       GPIO_Pin_3
#define KEY_5_PIN       GPIO_Pin_4
#define KEY_6_PIN       GPIO_Pin_5
#define KEY_7_PIN       GPIO_Pin_6
#define KEY_8_PIN       GPIO_Pin_7
#define KEY_9_PIN       GPIO_Pin_8
#define KEY_0_PIN       GPIO_Pin_9
#define KEY_ENTER_PIN   GPIO_Pin_18
#define KEY_DELETE_PIN  GPIO_Pin_19
#define KEY_CTRL_PIN    GPIO_Pin_20
#define KEY_ALT_PIN     GPIO_Pin_21
#define KEY_WIN_PIN     GPIO_Pin_22

#define KEY_PORT        GPIOB
#define KEY_GPIO_CLK    RCC_GPIOB

// 按键索引（用于bitmap的位位置）
#define KEY_INDEX_1      0
#define KEY_INDEX_2      1
#define KEY_INDEX_3      2
#define KEY_INDEX_4      3
#define KEY_INDEX_5      4
#define KEY_INDEX_6      5
#define KEY_INDEX_7      6
#define KEY_INDEX_8      7
#define KEY_INDEX_9      8
#define KEY_INDEX_0      9
#define KEY_INDEX_ENTER  10
#define KEY_INDEX_DELETE 11
#define KEY_INDEX_CTRL   12
#define KEY_INDEX_ALT    13
#define KEY_INDEX_WIN    14

// 按键按下/释放的电平（根据你的硬件电路）
// 如果你的按键按下时PB1为低电平（常见上拉接法），则：
#define KEY_PRESSED     0
#define KEY_RELEASED    1

extern volatile uint8_t g_key_changed;   

void KeyPad_Init(void);
uint8_t KeyPad_Scan(void);
uint16_t KeyPad_GetBitmap(void);

#endif