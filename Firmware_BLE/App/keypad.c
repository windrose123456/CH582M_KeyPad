#include "keypad.h"
// 矩阵扫描与防抖。管理所有按键的物理读取、消抖状态机。

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



