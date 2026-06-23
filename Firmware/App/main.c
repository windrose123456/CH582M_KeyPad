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

    uint8_t USB_Data[10] = { 0 };
    USB_Data[0] = 0x05;
    USB_Data[1] = 0x10;
    USB_Data[2] = 0x20;
    USB_Data[3] = 0x11;

    while(1)
    {
        // 1. 调用扫描函数（建议放在1ms定时器中断中，这里只是演示）
        if (KeyPad_Scan()) {
            // 2. 如果状态变化，获取当前状态
            uint8_t keyState = KeyPad_GetState(0);
            
            // 3. 仅当USB活跃时才发送
            if (HID_IsReady()) {
                if (keyState == KEY_PRESSED) {
                    // 发送按键按下：假设键值为 'A' (0x04)
                    DevHIDReport(USB_Data, 1);
                } else {
                    // 发送按键释放：全部清零
                    DevHIDReport(USB_Data, 1);
                }
            }
        }
        
        // 其他任务...
        mDelaymS(1);
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
