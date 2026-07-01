/********************************** (C) COPYRIGHT *******************************
 * File Name          : Main.c
 * Author             : WCH
 * Version            : V1.1
 * Date               : 2022/01/25
 * Description        : 模拟兼容HID设备
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

#include "CH58x_common.h"
#include "hid_report.h"
#include "keypad.h"

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
 * @fn      DebugInit
 *
 * @brief   调试初始化
 *
 * @return  none
 */
void DebugInit(void)
{
    GPIOA_SetBits(GPIO_Pin_9);
    GPIOA_ModeCfg(GPIO_Pin_8, GPIO_ModeIN_PU);
    GPIOA_ModeCfg(GPIO_Pin_9, GPIO_ModeOut_PP_5mA);
    UART1_DefInit();
}

void SysTick_Init(void) {
    // SysTick_Config 的参数是两次中断之间的时钟周期数
    // FREQ_SYS / 1000 = 60000，即 60M 个时钟周期计满一次，时间为 1ms[reference:3][reference:4]
    SysTick_Config(FREQ_SYS / 1000);
}

// SysTick 的中断服务函数，函数名是固定的
__INTERRUPT __HIGH_CODE void SysTick_Handler(void) {
    SysTick->SR = 0; // 清除中断标志，这是必须的操作[reference:5]
    
    // static uint16_t count = 0; // 测试确实为1ms中断一次
    // count++;
    // if(count >= 1000)
    // {
    //     printf("1111\n");
    //     count = 0;
    // }
    // 扫描按键，若状态变化则返回非0
    if (KeyPad_Scan()) {
        g_key_changed = 1;   // 置位变化标志
    }
}

/*********************************************************************
 * @fn      main
 *
 * @brief   主函数
 *
 * @return  none
 */
int main()
{
    uint8_t s;
    SetSysClock(CLK_SOURCE_PLL_60MHz);

    DebugInit();        //配置串口1用来prinft来debug
    printf("start\n");

    HID_InitUSBBuffer();

    USB_DeviceInit();

    PFIC_EnableIRQ(USB_IRQn);       //启用中断向量
    mDelaymS(100);

    KeyPad_Init();
    SysTick_Init();   // 配置 1ms 定时中断，加入蓝牙后需删除，KeyPad_Scan()改为在 TMOS 任务中定时调用

    while(1)
    {
        // 检查按键是否发生变化
        if (g_key_changed) {
            g_key_changed = 0;   // 清除标志
            
            // 只有USB就绪时才发送报告
            if (HID_IsReady()) {
                uint16_t bitmap = KeyPad_GetBitmap();
                uint8_t report[2];
                report[0] = bitmap & 0xFF;
                report[1] = (bitmap >> 8) & 0xFF;
                DevHIDReport(report, 2);
            }
        }
        
        __WFI();
    }
}

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
