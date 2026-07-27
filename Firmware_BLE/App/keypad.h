#ifndef __KEYPAD_H__
#define __KEYPAD_H__

#include "CH58x_common.h"

// 按键数量
#define KEY_NUM         15
// 按键扫描频率
#define KEYSCAN_INTERVAL_TICK     3    // 3 × 0.625ms ≈ 1.875ms ≈ 533Hz

/*
A1 A15  A7 B8   7 8 9 DELETE
A2 A13 A8 B15   4 5 6 ALT
A3 A12 A9       1 2 3
A14 A11 B9 B14  0 CTRL WIN ENTER
*/

#define GPIOA 0
#define GPIOB 1

// 15个按键的引脚定义
#define KEY_1_PORT      GPIOA
#define KEY_1_PIN       GPIO_Pin_3

#define KEY_2_PORT      GPIOA
#define KEY_2_PIN       GPIO_Pin_12

#define KEY_3_PORT      GPIOA
#define KEY_3_PIN       GPIO_Pin_9

#define KEY_4_PORT      GPIOA
#define KEY_4_PIN       GPIO_Pin_2

#define KEY_5_PORT      GPIOA
#define KEY_5_PIN       GPIO_Pin_13

#define KEY_6_PORT      GPIOA
#define KEY_6_PIN       GPIO_Pin_8

#define KEY_7_PORT      GPIOA
#define KEY_7_PIN       GPIO_Pin_1

#define KEY_8_PORT      GPIOA
#define KEY_8_PIN       GPIO_Pin_14

#define KEY_9_PORT      GPIOA
#define KEY_9_PIN       GPIO_Pin_7

#define KEY_0_PORT      GPIOA
#define KEY_0_PIN       GPIO_Pin_15

#define KEY_ENTER_PORT  GPIOB
#define KEY_ENTER_PIN   GPIO_Pin_14

#define KEY_DELETE_PORT GPIOB
#define KEY_DELETE_PIN  GPIO_Pin_8

#define KEY_CTRL_PORT   GPIOA
#define KEY_CTRL_PIN    GPIO_Pin_11

#define KEY_ALT_PORT    GPIOB
#define KEY_ALT_PIN     GPIO_Pin_15

#define KEY_WIN_PORT    GPIOB
#define KEY_WIN_PIN     GPIO_Pin_9

#define KEY_GPIOA_CLK    RCC_GPIOA
#define KEY_GPIOB_CLK    RCC_GPIOB

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

// 按键配置结构体
typedef struct {
    uint8_t  port;   // GPIOA 或 GPIOB
    uint32_t pin;
} KeyPinConfig_t;

// 引脚配置数组（顺序与 KEY_INDEX 对应）
static const KeyPinConfig_t key_config[KEY_NUM] = {
    {KEY_1_PORT,      KEY_1_PIN},      // index 0
    {KEY_2_PORT,      KEY_2_PIN},      // index 1
    {KEY_3_PORT,      KEY_3_PIN},      // index 2
    {KEY_4_PORT,      KEY_4_PIN},      // index 3
    {KEY_5_PORT,      KEY_5_PIN},      // index 4
    {KEY_6_PORT,      KEY_6_PIN},      // index 5
    {KEY_7_PORT,      KEY_7_PIN},      // index 6
    {KEY_8_PORT,      KEY_8_PIN},      // index 7
    {KEY_9_PORT,      KEY_9_PIN},      // index 8
    {KEY_0_PORT,      KEY_0_PIN},      // index 9
    {KEY_ENTER_PORT,  KEY_ENTER_PIN},  // index 10
    {KEY_DELETE_PORT, KEY_DELETE_PIN}, // index 11
    {KEY_CTRL_PORT,   KEY_CTRL_PIN},   // index 12
    {KEY_ALT_PORT,    KEY_ALT_PIN},    // index 13
    {KEY_WIN_PORT,    KEY_WIN_PIN}     // index 14
};

extern volatile uint8_t g_key_changed;   

void KeyPad_Init(void);
uint8_t KeyPad_Scan(void);
uint16_t KeyPad_GetBitmap(void);

#endif