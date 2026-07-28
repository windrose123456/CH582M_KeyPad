#include "ec11.h"
#include <stdbool.h>
#include "CONFIG.h"
#include "keypad.h"
#include "main.h"

/* ======================== 静态变量 ======================== */
// 旋转相关
static volatile int16_t s_step = 0;                     // 累计步数

// 按键相关
static volatile EC11_KeyState_t s_key_state = EC11_KEY_RELEASED;
uint32_t EC11_key_press_time = 0;

// 加速度相关
volatile uint32_t last_enc_time = 0;    // 上一次A相中断时间


/* ======================== 内部函数 ======================== */
static uint8_t ReadPin(uint32_t pin) {
    return (GPIOB_ReadPortPin(pin) ? 1 : 0);
}

/* ======================== 公开函数实现 ======================== */

void EC11_Init(void) {
    // 配置为带上拉输入（按键按下接地，编码器输出一般为开漏或推挽，上拉保证电平稳定）
    GPIOB_ModeCfg(EC11_A_PIN, GPIO_ModeIN_PU);
    GPIOB_ModeCfg(EC11_B_PIN, GPIO_ModeIN_PU);
    GPIOB_ModeCfg(EC11_D_PIN, GPIO_ModeIN_PU);

    // A相配置为上升沿+下降沿中断
    GPIOB_ITModeCfg(EC11_A_PIN, GPIO_ITMode_FallEdge);
    GPIOB_ITModeCfg(EC11_D_PIN, GPIO_ITMode_RiseEdge);
    PFIC_EnableIRQ(GPIO_B_IRQn);
}

// ==================== 获取接口 ====================
int16_t EC11_GetStep(void) {
    return s_step;
}

EC11_KeyState_t EC11_GetKeyState(void) {
    return s_key_state;
}

void EC11_ResetStep(void) {
    s_step = 0;
}

void EC11_ResetKey(void) {
    s_key_state = EC11_KEY_RELEASED;
}

__INTERRUPT __HIGH_CODE void GPIOB_IRQHandler(void)
{
    if(enter_sleep_flag == 1 
        && GPIOB_ReadITFlagBit(KEY_ENTER_PIN | KEY_DELETE_PIN | KEY_ALT_PIN | KEY_WIN_PIN))
    {
        GPIOB_ClearITFlagBit(KEY_ENTER_PIN | KEY_DELETE_PIN | KEY_ALT_PIN | KEY_WIN_PIN);
        return;
    }

    if (GPIOB_ReadITFlagBit(EC11_A_PIN))
    {
        GPIOB_ClearITFlagBit(EC11_A_PIN);
        // 读取B相当前电平判断方向
        s_step = ReadPin(EC11_B_PIN) ? -1 : 1;
    }
    if (GPIOB_ReadITFlagBit(EC11_D_PIN))
    {
        GPIOB_ClearITFlagBit(EC11_D_PIN);
        if (TMOS_GetSystemClock() - EC11_key_press_time > 300) 
        {
            EC11_key_press_time = TMOS_GetSystemClock();
            s_key_state = EC11_KEY_PRESSED;
        }
    }
}