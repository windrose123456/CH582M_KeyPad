#ifndef __EC11_H__
#define __EC11_H__

#include "CH58x_common.h"

/* ======================== 硬件引脚定义 ======================== */
#define EC11_A_PIN      GPIO_Pin_2   // 编码器 A 相
#define EC11_B_PIN      GPIO_Pin_3   // 编码器 B 相
#define EC11_D_PIN      GPIO_Pin_0   // 编码器按键（按下接地）

/* ======================== 方向与按键状态 ======================== */
typedef enum {
    EC11_DIR_NONE = 0,
    EC11_DIR_CW   = 1,   // 顺时针（正转）
    EC11_DIR_CCW  = -1   // 逆时针（反转）
} EC11_Direction_t;

typedef enum {
    EC11_KEY_RELEASED = 0,
    EC11_KEY_PRESSED  = 1
} EC11_KeyState_t;

/* ======================== 外部接口 ======================== */
void EC11_Init(void);                       // 初始化 GPIO
void EC11_Scan(void);                       // 定时扫描（建议 1ms 调用一次）
int16_t EC11_GetStep(void);                 // 获取累计旋转步数（可正可负）
EC11_Direction_t EC11_GetDirection(void);   // 获取最近一次旋转方向
EC11_KeyState_t EC11_GetKeyState(void);     // 获取按键状态（已消抖）
void EC11_ResetStep(void);                  // 重置步数为 0

#endif