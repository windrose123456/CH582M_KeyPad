#include "keypad.h"
// 矩阵扫描与防抖。管理所有按键的物理读取、消抖状态机。

// 引脚数组，顺序与索引对应
static const uint32_t key_pins[KEY_NUM] = {
    KEY_1_PIN,   // index 0
    KEY_2_PIN,   // index 1
    KEY_3_PIN,   // index 2
    KEY_4_PIN,   // index 3
    KEY_5_PIN,   // index 4
    KEY_6_PIN,   // index 5
    KEY_7_PIN,   // index 6
    KEY_8_PIN,   // index 7
    KEY_9_PIN,   // index 8
    KEY_0_PIN,   // index 9   
    KEY_ENTER_PIN, // index 10 // PB18
    KEY_DELETE_PIN,// index 11 // PB19
    KEY_CTRL_PIN,  // index 12 // PB20
    KEY_ALT_PIN,   // index 13 // PB21
    KEY_WIN_PIN    // index 14 // PB22
};

// 按键状态变化标志
volatile uint8_t g_key_changed = 0;

// 按键状态数组（当前状态）
static uint8_t s_keyState[KEY_NUM] = {KEY_RELEASED};

// 防抖计数器（用于状态机）
static uint8_t s_debounceCount[KEY_NUM] = {0};

// 读取某个引脚的逻辑电平
static uint8_t KeyPad_ReadPin(uint32_t pin) {
    return (GPIOB_ReadPort() & pin) ? 1 : 0;   // 高电平为1，低电平为0
}

/**
 * @brief 初始化所有按键GPIO（输入上拉）
 */
void KeyPad_Init(void) {
    for (int i = 0; i < KEY_NUM; i++) {
        GPIOB_ModeCfg(key_pins[i], GPIO_ModeIN_PU);   // 上拉输入
        s_keyState[i] = KeyPad_ReadPin(key_pins[i]); // 读取初始电平
        s_debounceCount[i] = 0;
    }

    // 注册TMOS任务
    hidEmuTaskId = TMOS_ProcessEventRegister(hidEmuProcessEvent);
    // 启动1ms按键扫描
    tmos_start_task(hidEmuTaskId, START_KEYSCAN_EVT, 3);  // 16个0.625ms = 10ms
}

/**
 * @brief 扫描所有按键，带防抖状态机（建议1ms调用一次）
 * @return 1 表示有任意按键状态发生变化，0 表示无变化
 */
uint8_t KeyPad_Scan(void) {
    uint8_t changed = 0;
    for (int i = 0; i < KEY_NUM; i++) {
        uint8_t current = KeyPad_ReadPin(key_pins[i]);
        if (current == s_keyState[i]) {
            s_debounceCount[i] = 0;   // 稳定，计数器清零
        } else {
            s_debounceCount[i]++;     // 抖动，累加
            if (s_debounceCount[i] >= 5) {   // 连续5ms稳定
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



