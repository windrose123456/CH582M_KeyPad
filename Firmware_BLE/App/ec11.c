#include "ec11.h"
#include <stdbool.h>
#include "CONFIG.h"

/* ======================== 静态变量 ======================== */
// 旋转相关
static volatile int16_t s_step = 0;                     // 累计步数
static volatile EC11_Direction_t s_direction = EC11_DIR_NONE;

// 上次 A/B 电平（用于边沿检测）
static uint8_t s_last_A = 0;
static uint8_t s_last_B = 0;

// 按键相关（带消抖）
static volatile EC11_KeyState_t s_key_state = EC11_KEY_RELEASED;
static uint8_t s_key_prev_raw = 0;          // 上次原始电平
static uint8_t s_key_debounce_cnt = 0;      // 消抖计数器

// 加速度相关（需要系统 1ms 滴答）
extern volatile uint32_t g_sys_tick_ms;     // 在 main.c 或定时器中断中更新
static uint32_t s_last_edge_time = 0;       // 上一次边沿触发的时间戳（ms）

// 边沿锁定防抖（检测到边沿后，短时内忽略新边沿）
static uint32_t s_edge_lock_until = 0;      // 锁定截止时间戳（ms）

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

    // 读取初始状态
    s_last_A = ReadPin(EC11_A_PIN);
    s_last_B = ReadPin(EC11_B_PIN);
    s_key_prev_raw = ReadPin(EC11_D_PIN);
    s_key_state = (s_key_prev_raw == 0) ? EC11_KEY_PRESSED : EC11_KEY_RELEASED;
    s_key_debounce_cnt = 0;
    s_edge_lock_until = 0;
}

void EC11_Scan(void) {
    // ==================== 1. 旋转检测（A 相双边沿 + 加速度） ====================
    uint8_t cur_A = ReadPin(EC11_A_PIN);
    uint8_t cur_B = ReadPin(EC11_B_PIN);

    // 检测 A 相是否发生变化（上升沿或下降沿）
    if (cur_A != s_last_A) {
        // 防抖：若当前时间小于锁定截止时间，则忽略此次边沿
        uint32_t now = TMOS_GetSystemClock();
        //uint32_t now = GetSysClock();
        if (now < s_edge_lock_until) {
            s_last_A = cur_A;          // 仍需更新上一次电平，否则会重复触发
            // 不进行步进计数，直接返回（但需更新 s_last_A 防止下次误判）
        } else {
            // ---------- 方向判断 ----------
            int8_t step_dir = 0;
            if (cur_B == 1) {
                // B 为高 → 顺时针（根据实际硬件可能相反，可交换此处判断）
                s_direction = EC11_DIR_CW;
                step_dir = 1;
            } else {
                s_direction = EC11_DIR_CCW;
                step_dir = -1;
            }

            // ---------- 加速度计算 ----------
            uint32_t interval = now - s_last_edge_time;
            s_last_edge_time = now;

            int8_t step_mult = 1;   // 默认步进倍数
            if (interval < 8) {          // 极快速（<8ms/格）
                step_mult = 10;
            } else if (interval < 20) {  // 快速
                step_mult = 5;
            } else if (interval < 50) {  // 中速
                step_mult = 2;
            } else {                     // 慢速（≥50ms/格）
                step_mult = 1;
            }

            // 累加步数（方向 × 倍数）
            s_step += (step_dir * step_mult);

            // ---------- 边沿锁定（4*0.625ms 内忽略新边沿，消除机械抖动） ----------
            s_edge_lock_until = now + 4;

            // 更新上一次电平（已经变化，直接赋值）
            s_last_A = cur_A;
        }
    } else {
        // 电平未变化，维持上次状态（无需操作）
    }

    // 保存 B 相状态（本次未使用，但保留以备扩展）
    s_last_B = cur_B;

    // ==================== 2. 按键检测（带消抖） ====================
    uint8_t cur_D = ReadPin(EC11_D_PIN);
    if (cur_D != s_key_prev_raw) {
        s_key_debounce_cnt++;
        if (s_key_debounce_cnt >= 3) {   // 连续 5.4ms 稳定则认为有效
            s_key_state = (cur_D == 0) ? EC11_KEY_PRESSED : EC11_KEY_RELEASED;
            s_key_prev_raw = cur_D;
            s_key_debounce_cnt = 0;
        }
    } else {
        s_key_debounce_cnt = 0;          // 电平稳定，清零计数器
    }
}

// ==================== 获取接口 ====================
int16_t EC11_GetStep(void) {
    return s_step;
}

EC11_Direction_t EC11_GetDirection(void) {
    return s_direction;
}

EC11_KeyState_t EC11_GetKeyState(void) {
    return s_key_state;
}

void EC11_ResetStep(void) {
    s_step = 0;
}