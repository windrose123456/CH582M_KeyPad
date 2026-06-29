#include "keypad.h"
// 矩阵扫描与防抖。管理所有按键的物理读取、消抖状态机。

#define KEY_NUM         10

// 按键状态数组（当前状态）
static uint8_t s_keyState[1] = {KEY_RELEASED};

// 防抖计数器（用于状态机）
static uint8_t s_debounceCount[1] = {0};

// 读取当前引脚电平（转换为逻辑值）
static uint8_t KeyPad_ReadPin(void)
{
    // GPIOB的输入数据寄存器是 R32_PB_IN
    // 或者使用库函数：GPIOB_ReadPortPin(KEY_PIN)
    uint8_t pinLevel = (GPIOB_ReadPort() & KEY_PIN) ? 1 : 0;
    return pinLevel;
}

/**
 * @brief 初始化按键GPIO
 *        配置PB1为输入，上拉模式（适用于按键接GND）
 */
void KeyPad_Init(void)
{    
    // 1. 配置PB1为输入，上拉
    //    PB1 使用 GPIO_ModeIN_PU（输入上拉）
    GPIOB_ModeCfg(KEY_PIN, GPIO_ModeIN_PU);
    
    // 2. 读取一次初始状态，用于后续防抖
    s_keyState[0] = KeyPad_ReadPin();
    s_debounceCount[0] = 0;
}

/**
 * @brief 扫描单个按键（非阻塞防抖状态机）
 *        建议在1ms定时器中断或主循环中每1ms调用一次
 * @return 1 如果按键状态发生变化，0 否则
 */
uint8_t KeyPad_Scan(void)
{
    uint8_t changed = 0;
    uint8_t currentPin = KeyPad_ReadPin();
    
    // 状态机逻辑：
    // 如果当前读取的电平 等于 当前认定的状态，计数器清零
    // 如果 不等于，计数器累加，累加到5次（5ms）确认状态变化
    if (currentPin == s_keyState[0]) {
        // 电平稳定，计数器清零
        s_debounceCount[0] = 0;
    } else {
        // 电平发生抖动，计数器累加
        s_debounceCount[0]++;
        // 如果连续5ms检测到相同的“新”电平，则认为状态稳定
        if (s_debounceCount[0] >= 5) {
            // 更新状态
            s_keyState[0] = currentPin;
            s_debounceCount[0] = 0;
            changed = 1;   // 标记状态发生了变化
        }
    }
    
    return changed;
}

/**
 * @brief 获取指定按键的当前状态（已消抖）
 * @param key_index 按键索引（0 ~ KEY_NUM-1）
 * @return KEY_PRESSED 或 KEY_RELEASED
 */
uint8_t KeyPad_GetState(uint8_t key_index)
{
    // 虽然现在只有一个键，但为了将来扩展，保留key_index参数
    if (key_index == 0) {
        return s_keyState[0];
    }
    return KEY_RELEASED;  // 无效索引默认返回释放
}

uint16_t KeyPad_GetBitmap(void)
{
    uint16_t bitmap = 0;
    for (int i = 0; i < KEY_NUM; i++) {
        if (s_keyState[i] == KEY_PRESSED) {
            bitmap |= (1 << i);
        }
    }
    return bitmap;
}