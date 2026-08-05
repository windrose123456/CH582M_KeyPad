/********************************** (C) COPYRIGHT *******************************
 * File Name          : main.c
 * Author             : 
 * Version            : V0.1
 * Date               : 2026/08/04
 * Description        : 三模键盘应用主函数及任务系统初始化
 *******************************************************************************/

/* 头文件包含 */
#include "CONFIG.h"
#include "HAL.h"
#include "main.h"
#include "hid_report.h"
#include "sleep.h"

/*********************************************************************
 * GLOBAL TYPEDEFS
 */
__attribute__((aligned(4))) uint32_t MEM_BUF[BLE_MEMHEAP_SIZE / 4];

#if(defined(BLE_MAC)) && (BLE_MAC == TRUE)
const uint8_t MacAddr[6] = {0x84, 0xC2, 0xE4, 0x03, 0x02, 0x02};
#endif

/*********************************************************************
 * @fn      USB_Dev_Wakeup
 *
 * @brief   设备模式唤醒主机
 *
 * @return  none
 */
void USB_Dev_Wakeup(void)
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
        Sleep_EnterCheck();
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

    HID_Init();

    // 还需要看门狗
    // 还需要实现：USB 和 BLE ，检测不到USB就禁用USB
    // 看看下电模式是否可以通过GPIO中断唤醒，现在唤醒初始化已经等同于重新上电，或者看看能不能改成RAM供电也不保留了

    last_send_date_tick = TMOS_GetSystemClock();
    printf("all device init done\n");
    Main_Circulation();
}

/******************************** endfile @ main ******************************/




