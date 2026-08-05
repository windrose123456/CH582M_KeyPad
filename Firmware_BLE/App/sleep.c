#include "HAL.h"
#include "ec11.h"
#include "keypad.h"
#include "hid_report.h"

#define SLEEP_TIMEOUT_MS 960000 // 960000 是 10min，8000 是 用于5s测试

volatile uint32_t last_send_date_tick = 0; // 低功耗模式使用
volatile uint8_t enter_sleep_flag = 0; // 进入sleep mode 置1，唤醒中断置0
volatile uint8_t wakeup_source = 0;

void Sleep_WakeupConfig(void);
void Enter_SleepMode(void);
void Wakeup_Reinit(void);

void Sleep_EnterCheck(void) {
    if (TMOS_GetSystemClock() - last_send_date_tick > SLEEP_TIMEOUT_MS) {
        printf("Entering sleep mode...");
        Enter_SleepMode();           // 阻塞在这里，直到唤醒
        Wakeup_Reinit();             // 唤醒后执行
        last_send_date_tick = TMOS_GetSystemClock();
        printf("Wakeup complete, wakeup_source = %d\n", wakeup_source);
    }
}

// 唤醒中断配置，使用sleep模式
void Sleep_WakeupConfig(void) {
    GPIOA_ModeCfg(GPIO_Pin_All, GPIO_ModeIN_PU);
    GPIOB_ModeCfg(GPIO_Pin_All, GPIO_ModeIN_PU);

    for (int i = 0; i < KEY_NUM; i++) {
        uint8_t port = key_config[i].port;
        uint32_t pin = key_config[i].pin;

        if (port == GPIOA) {
            GPIOA_ModeCfg(pin, GPIO_ModeIN_PU); // 上拉输入
            GPIOA_ITModeCfg(pin, GPIO_ITMode_FallEdge);
        } else {
            GPIOB_ModeCfg(pin, GPIO_ModeIN_PU);
            GPIOB_ITModeCfg(pin, GPIO_ITMode_FallEdge);
        }
    }
    GPIOB_ModeCfg(EC11_A_PIN, GPIO_ModeIN_PU);
    GPIOB_ModeCfg(EC11_B_PIN, GPIO_ModeIN_PU);
    GPIOB_ModeCfg(EC11_D_PIN, GPIO_ModeIN_PU);
    GPIOB_ITModeCfg(EC11_A_PIN, GPIO_ITMode_FallEdge);
    GPIOB_ITModeCfg(EC11_D_PIN, GPIO_ITMode_RiseEdge);

    GPIOA_ClearITFlagBit(0xFFFF);       // 清除挂起的中断标志
    GPIOB_ClearITFlagBit(0xFFFF);
    PFIC_EnableIRQ(GPIO_A_IRQn);          // 使能 GPIO 中断
    PFIC_EnableIRQ(GPIO_B_IRQn);
    PWR_PeriphWakeUpCfg(ENABLE, RB_SLP_GPIO_WAKE, Long_Delay); // 电源管理单元启用 GPIO 唤醒
}

void Enter_SleepMode(void) {
    USB_DeviceDisable();

    // ② 关闭其他外设时钟
    // 关闭 UART、SPI、定时器等
    // R8_CLK_SYS_CFG = 0x00;  // 具体值参考手册

    // ③ 配置唤醒源
    Sleep_WakeupConfig();

    enter_sleep_flag = 1;

    // 进入睡眠，只保留必要的 RAM
    LowPower_Sleep(RB_PWR_RAM30K | RB_PWR_RAM2K);

    //LowPower_Sleep 函数内部为了确保可靠唤醒，会临时提高高频时钟（HSE）的偏置电流。因此，唤醒后必须调用 HSECFG_Current(HSE_RCur_100); 将其恢复为额定电流，否则功耗会偏高
    // 被唤醒后，立即恢复高频时钟电流
    HSECFG_Current(HSE_RCur_100);
    enter_sleep_flag = 0;
}

void Wakeup_Reinit(void) {
    // ① 关闭唤醒中断（防止抖动重复触发）
    PFIC_DisableIRQ(GPIO_A_IRQn);
    PFIC_DisableIRQ(GPIO_B_IRQn);

#if(defined(DCDC_ENABLE)) && (DCDC_ENABLE == TRUE)
    PWR_DCDCCfg(ENABLE);
#endif
    SetSysClock(CLK_SOURCE_PLL_60MHz);
    mDelaymS(10);                         // 等待时钟稳定
#if(defined(HAL_SLEEP)) && (HAL_SLEEP == TRUE)
    GPIOA_ModeCfg(GPIO_Pin_All, GPIO_ModeIN_PU);
    GPIOB_ModeCfg(GPIO_Pin_All, GPIO_ModeIN_PU);
#endif
#ifdef DEBUG 
    GPIOB_SetBits(bTXD0);
    GPIOB_ModeCfg(bTXD0, GPIO_ModeOut_PP_5mA);
    UART0_DefInit();
#endif
    CH58X_BLEInit();
    HAL_Init();
    HID_Init();
}

__INTERRUPT __HIGH_CODE void GPIOA_IRQHandler(void)
{
    if (enter_sleep_flag == 1 && GPIOA_ReadITFlagBit(KEY_1_PIN | KEY_2_PIN 
        | KEY_3_PIN | KEY_4_PIN | KEY_5_PIN | KEY_6_PIN | KEY_7_PIN 
        | KEY_8_PIN | KEY_9_PIN | KEY_0_PIN | KEY_CTRL_PORT))
    {
        GPIOA_ClearITFlagBit(KEY_1_PIN | KEY_2_PIN | KEY_3_PIN | KEY_4_PIN 
            | KEY_5_PIN | KEY_6_PIN | KEY_7_PIN | KEY_8_PIN 
            | KEY_9_PIN | KEY_0_PIN | KEY_CTRL_PORT);
        wakeup_source = 1;  // 标记是 GPIOA 唤醒的
        return;
    }

    // if (GPIOA_ReadITFlagBit(TOUCH_IRQ_PIN))
    // {
    //     GPIOA_ClearITFlagBit(TOUCH_IRQ_PIN);
    //     // 发送指纹模块采集指令
    //     touch_irq_flag += 1;
    //     //printf("touch_irq\n");
    // }
}

__INTERRUPT __HIGH_CODE void GPIOB_IRQHandler(void)
{
    if(enter_sleep_flag == 1 
        && GPIOB_ReadITFlagBit(KEY_ENTER_PIN | KEY_DELETE_PIN | KEY_ALT_PIN | KEY_WIN_PIN))
    {
        GPIOB_ClearITFlagBit(KEY_ENTER_PIN | KEY_DELETE_PIN | KEY_ALT_PIN | KEY_WIN_PIN);
        wakeup_source = 2;  // 标记是 GPIOB 唤醒的
        return;
    }

    if (GPIOB_ReadITFlagBit(EC11_A_PIN))
    {
        GPIOB_ClearITFlagBit(EC11_A_PIN);
        EC11_SetStep();
    }
    if (GPIOB_ReadITFlagBit(EC11_D_PIN))
    {
        GPIOB_ClearITFlagBit(EC11_D_PIN);
        EC11_SetKeyState();
    }
}