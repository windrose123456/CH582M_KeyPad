/********************************** (C) COPYRIGHT *******************************
 * File Name          : main.c
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2020/08/06
 * Description        : 蓝牙键盘应用主函数及任务系统初始化
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

/******************************************************************************/
/* 头文件包含 */
#include "CONFIG.h"
#include "HAL.h"
#include "hiddev.h"
#include "hidkbd.h"
#include "keypad.h"
#include "hid_report.h"
#include "fingerprint_drv.h"
#include "ec11.h"
#include "ws2812b.h"
#include "battery.h"

volatile uint32_t last_key_tick = 0; // 低功耗模式使用

/*********************************************************************
 * GLOBAL TYPEDEFS
 */
__attribute__((aligned(4))) uint32_t MEM_BUF[BLE_MEMHEAP_SIZE / 4];

#if(defined(BLE_MAC)) && (BLE_MAC == TRUE)
const uint8_t MacAddr[6] = {0x84, 0xC2, 0xE4, 0x03, 0x02, 0x02};
#endif

/*********************************************************************
 * @fn      DevWakeup
 *
 * @brief   设备模式唤醒主机
 *
 * @return  none
 */
void DevWakeup(void)
{
    R16_PIN_ANALOG_IE &= ~(RB_PIN_USB_DP_PU);
    R8_UDEV_CTRL |= RB_UD_LOW_SPEED;
    mDelaymS(2);
    R8_UDEV_CTRL &= ~RB_UD_LOW_SPEED;
    R16_PIN_ANALOG_IE |= RB_PIN_USB_DP_PU;
}

/*********************************************************************
 * @fn      Main_Circulation
 *
 * @brief   主循环
 *
 * @return  none
 */
__HIGH_CODE
__attribute__((noinline))
void Main_Circulation()
{
    while(1)
    {
        TMOS_SystemProcess();
        // ===== 检查是否该休眠 =====
        if (SYS_GetSysTickCnt() - last_key_tick > SLEEP_TIMEOUT_MS) {
            printf("Entering sleep mode...\n");
            Enter_SleepMode();           // 阻塞在这里，直到唤醒
            Wakeup_Reinit();             // 唤醒后执行
            last_key_tick = TMOS_GetSystemClock();
            printf("Wakeup complete\n");
        }
    }
}

/*********************************************************************
 * @fn      main
 *
 * @brief   主函数
 *
 * @return  none
 */
int main(void)
{
#if(defined(DCDC_ENABLE)) && (DCDC_ENABLE == TRUE)
    PWR_DCDCCfg(ENABLE);
#endif
    SetSysClock(CLK_SOURCE_PLL_60MHz);
#if(defined(HAL_SLEEP)) && (HAL_SLEEP == TRUE)
    GPIOA_ModeCfg(GPIO_Pin_All, GPIO_ModeIN_PU);
    GPIOB_ModeCfg(GPIO_Pin_All, GPIO_ModeIN_PU);
#endif
#ifdef DEBUG 
    // GPIOA_SetBits(bTXD1);
    // GPIOA_ModeCfg(bTXD1, GPIO_ModeOut_PP_5mA);
    // UART1_DefInit();
    GPIOB_SetBits(bTXD0);
    GPIOB_ModeCfg(bTXD0, GPIO_ModeOut_PP_5mA);
    UART0_DefInit();
#endif
    PRINT("%s\n", VER_LIB);
    CH58X_BLEInit();
    HAL_Init();
    GAPRole_PeripheralInit();
    HidDev_Init();
    HidEmu_Init();

    // USB init
    HID_InitUSBBuffer();
    USB_DeviceInit();
    PFIC_EnableIRQ(USB_IRQn);       //启用中断向量
    mDelaymS(100);

    KeyPad_Init();
    EC11_Init();
    // Battery_Init();
    //FP_Init();
    // WS2812B_Init(); // 灯控有问题，不要开启

    last_key_tick = SYS_GetSysTickCnt();
    printf("all device init done\n");
    Main_Circulation();

    //在不同模式中，按键扫描频率可以不一致，BLE没必要这么快
}

/******************************** endfile @ main ******************************/


/*********************************************************************
 * @fn      DevEP1_OUT_Deal
 *
 * @brief   端点1数据处理，收到数据后取反再发出去。用户自行更改。
 *
 * @return  none
 */
void DevEP1_OUT_Deal(uint8_t l)
{ /* 用户可自定义 */
    uint8_t i;

    for(i = 0; i < l; i++)
    {
        pEP1_IN_DataBuf[i] = ~pEP1_OUT_DataBuf[i];
    }
    DevEP1_IN_Deal(l);
}


/*********************************************************************
 * @fn      USB_IRQHandler
 *
 * @brief   USB中断函数
 *
 * @return  none
 */
__attribute__((interrupt("WCH-Interrupt-fast")))
__attribute__((section(".highcode")))
void USB_IRQHandler(void) /* USB中断服务程序,使用寄存器组1 */
{
    USB_DevTransProcess();
}


// 唤醒中断配置，使用sleep模式
void Sleep_WakeupConfig(void) {
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
}

void Enter_SleepMode(void) {
    // ① 关闭 USB（可选，看功耗要求）
    // 如果需要 USB 远程唤醒则保留 USB，否则关闭
    USB_Disable();

    // ② 关闭其他外设时钟
    // 关闭 UART、SPI、定时器等
    R8_CLK_SYS_CFG = 0x00;  // 具体值参考手册

    // ③ 配置唤醒源
    Sleep_WakeupConfig();

    // ④ 进入待机
    // CH582M 的低功耗进入方式
    LowPower_Sleep(LPM_IDLE);  // 或 LPM_STANDBY，看你的需求
}

volatile uint8_t wakeup_flag = 0;

void GPIO_IRQHandler(void) {
    if (GPIOA_GetITFlagBit(0xFFFF)) {
        GPIOA_ClearITFlagBit(0xFFFF);    // 清中断标志
        wakeup_flag = 1;                  // 标记唤醒事件
    }
}

void Wakeup_Reinit(void) {
    // ① 关闭唤醒中断（防止抖动重复触发）
    PFIC_DisableIRQ(GPIO_IRQn);

    // ② 重新初始化时钟（可能被降频或关闭）
    SetSysClock(CLK_SOURCE_PLL_60MHz);   // 恢复正常时钟
    mDelaymS(10);                         // 等待时钟稳定

    // ③ 重新初始化 USB
    USB_Init();
    // 注意：主机可能需要几秒重新枚举
    // 这期间不能发送 HID 报告

    // ④ 重新初始化 GPIO 为正常工作模式
    // 按键矩阵回到扫描模式
    KeyPad_Init();

    // ⑤ 重新初始化 EC11 编码器
    EC11_Init();

    // ⑥ 清除标志
    wakeup_flag = 0;
}