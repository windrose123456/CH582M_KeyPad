#include "keypad.h"
// 矩阵扫描与防抖。管理所有按键的物理读取、消抖状态机。

// 按键状态变化标志
volatile uint8_t g_key_changed = 0;

// 按键状态数组（当前状态）
static uint8_t s_keyState[KEY_NUM] = {KEY_RELEASED};

// 防抖计数器（用于状态机）
static uint8_t s_debounceCount[KEY_NUM] = {0};

// 读取某个引脚的逻辑电平
static uint8_t KeyPad_ReadPin(uint8_t port, uint32_t pin) {
    if (port == GPIOA) { // GPIOA
        return GPIOA_ReadPortPin(pin) ? 1 : 0;   // 高电平为1，低电平为0
    } else {         // GPIOB
        return GPIOB_ReadPortPin(pin) ? 1 : 0;
    }
}

/**
 * @brief 初始化所有按键GPIO（输入上拉）
 */
void KeyPad_Init(void) {
    for (int i = 0; i < KEY_NUM; i++) {
        uint8_t port = key_config[i].port;
        uint32_t pin = key_config[i].pin;

        if (port == GPIOA) {
            GPIOA_ModeCfg(pin, GPIO_ModeIN_PU); // 上拉输入
        } else {
            GPIOB_ModeCfg(pin, GPIO_ModeIN_PU);
        }

        s_keyState[i] = KeyPad_ReadPin(port, pin);  // 读取初始电平
        s_debounceCount[i] = 0;
    }
}

/**
 * @brief 扫描所有按键，带防抖状态机（建议1ms调用一次）
 * @return 1 表示有任意按键状态发生变化，0 表示无变化
 */
uint8_t KeyPad_Scan(void) {
    uint8_t changed = 0;
    for (int i = 0; i < KEY_NUM; i++) {
        uint8_t current = KeyPad_ReadPin(key_config[i].port, key_config[i].pin);
        if (current == s_keyState[i]) {
            s_debounceCount[i] = 0;   // 稳定，计数器清零
        } else {
            s_debounceCount[i]++;     // 抖动，累加
            if (s_debounceCount[i] >= 3) {   // 连续5.4ms稳定
                s_keyState[i] = current;
                s_debounceCount[i] = 0;
                changed = 1;          // 标记状态变化
            }
        }
    }
    return changed;
}

/**
 * @brief 获取当前按键位图（低位bit0对应0键，bit1对应1键...）
 */
uint16_t KeyPad_GetBitmap(void) {
    uint16_t bitmap = 0;
    for (int i = 0; i < KEY_NUM; i++) {
        if (s_keyState[i] == KEY_PRESSED) {
            bitmap |= (1 << i);
        }
    }
    return bitmap;
}



