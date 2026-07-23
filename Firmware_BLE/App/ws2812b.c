//灯光控制。管理背光、指示灯

#include "ws2812b.h"

// static void delay_ns(uint32_t ns) {
//     uint32_t cycles = (uint32_t)((uint64_t)ns * GetSysClock() / 1000000000);
//     for (volatile uint32_t i = 0; i < cycles; i++);
// }

// 延时指定个数的时钟周期（仅计数，不包含调用开销）
static inline void delay_cycles(uint32_t n) {
    __asm volatile (
        "1: addi %0, %0, -1\n"   // n = n - 1
        "bnez %0, 1b\n"          // if (n != 0) goto 1
        : "+r" (n)               // 输入输出寄存器
        :
        : "cc"                   // 条件码可能改变
    );
}

// 定义延时（单位：时钟周期）
#define DELAY_CYCLES(n)  delay_cycles((n) / 2)   // 因为每个循环2周期

static void ws2812b_send_bit(uint8_t bit) {
    if (bit) {
        GPIOB_SetBits(WS2812B_PIN);
        DELAY_CYCLES(48);   // 高800ns
        GPIOB_ResetBits(WS2812B_PIN);
        DELAY_CYCLES(27);   // 低450ns
    } else {
        GPIOB_SetBits(WS2812B_PIN);
        DELAY_CYCLES(24);   // 高400ns
        GPIOB_ResetBits(WS2812B_PIN);
        DELAY_CYCLES(51);   // 低850ns
    }
}


static void ws2812b_send_byte(uint8_t byte)
{
    // MSB first
    for (int i = 7; i >= 0; i--) {
        ws2812b_send_bit((byte >> i) & 1);
    }
}

void WS2812B_Init(void)
{
    GPIOB_ModeCfg(WS2812B_PIN, GPIO_ModeOut_PP_5mA);
    GPIOB_ResetBits(WS2812B_PIN);  // 初始低电平

    uint8_t led_data[15 * 3];
    for (int i = 0; i < 15; i++) {
        led_data[i*3 + 0] = 0;     // G
        led_data[i*3 + 1] = 255;   // R
        led_data[i*3 + 2] = 0;     // B
    }
    WS2812B_SendArray(led_data, sizeof(led_data));
}

// 发送多颗 LED 的数据
void WS2812B_SendArray(uint8_t *data, uint16_t len)
{
    PFIC_DisableAllIRQ();

    for (uint16_t i = 0; i < len; i++) {
        ws2812b_send_byte(data[i]);
    }

    GPIOB_ResetBits(WS2812B_PIN);
    DELAY_CYCLES(60000);

    PFIC_EnableAllIRQ();
}

// 加入TMOS参考，暂不考虑
// #define WS2812B_EVT    0x01

// static uint8_t myAppTask(uint8_t task_id, uint8_t event)
// {
//     switch (event)
//     {
//     case WS2812B_EVT:
//         // 在 TMOS 回调中直接调用即可
//         // 关中断时间极短（几颗 LED 几百微秒），不影响协议栈
//         WS2812B_Send(255, 0, 0);  // 红色
//         return EVENT_NO_READY;
//     default:
//         return EVENT_NO_READY;
//     }
// }

// // 触发
// tmos_start_task(myTaskID, WS2812B_EVT, 0);