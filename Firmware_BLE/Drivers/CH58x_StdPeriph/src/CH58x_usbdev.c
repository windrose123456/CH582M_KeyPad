/********************************** (C) COPYRIGHT *******************************
 * File Name          : CH58x_usbdev.c
 * Author             : WCH
 * Version            : V1.2
 * Date               : 2021/11/17
 * Description
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

#include "CH58x_common.h"

uint8_t *pEP0_RAM_Addr;
uint8_t *pEP1_RAM_Addr;
uint8_t *pEP2_RAM_Addr;
uint8_t *pEP3_RAM_Addr;

/*********************************************************************
 * @fn      USB_DeviceInit
 *
 * @brief   USB设备功能初始化，4个端点，8个通道。
 *
 * @param   none
 *
 * @return  none
 */
void USB_DeviceInit(void)
{
    R8_USB_CTRL = 0x00; // 先设定模式,取消 RB_UC_CLR_ALL

    R8_UEP4_1_MOD = RB_UEP4_RX_EN | RB_UEP4_TX_EN | RB_UEP1_RX_EN | RB_UEP1_TX_EN; // 端点4 OUT+IN,端点1 OUT+IN
    R8_UEP2_3_MOD = RB_UEP2_RX_EN | RB_UEP2_TX_EN | RB_UEP3_RX_EN | RB_UEP3_TX_EN; // 端点2 OUT+IN,端点3 OUT+IN

    R16_UEP0_DMA = (uint16_t)(uint32_t)pEP0_RAM_Addr;
    R16_UEP1_DMA = (uint16_t)(uint32_t)pEP1_RAM_Addr;
    R16_UEP2_DMA = (uint16_t)(uint32_t)pEP2_RAM_Addr;
    R16_UEP3_DMA = (uint16_t)(uint32_t)pEP3_RAM_Addr;

    R8_UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
    R8_UEP1_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK | RB_UEP_AUTO_TOG;
    R8_UEP2_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK | RB_UEP_AUTO_TOG;
    R8_UEP3_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK | RB_UEP_AUTO_TOG;
    R8_UEP4_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;

    R8_USB_DEV_AD = 0x00;
    R8_USB_CTRL = RB_UC_DEV_PU_EN | RB_UC_INT_BUSY | RB_UC_DMA_EN; // 启动USB设备及DMA，在中断期间中断标志未清除前自动返回NAK
    R16_PIN_ANALOG_IE |= RB_PIN_USB_IE | RB_PIN_USB_DP_PU;         // 防止USB端口浮空及上拉电阻
    R8_USB_INT_FG = 0xFF;                                          // 清中断标志
    R8_UDEV_CTRL = RB_UD_PD_DIS | RB_UD_PORT_EN;                   // 允许USB端口
    R8_USB_INT_EN = RB_UIE_SUSPEND | RB_UIE_BUS_RST | RB_UIE_TRANSFER;
}

/*********************************************************************
 * @fn      DevEP1_IN_Deal
 *
 * @brief   端点1数据上传
 *
 * @param   l   - 上传数据长度(<64B)
 *
 * @return  none
 */
void DevEP1_IN_Deal(uint8_t l)
{
    R8_UEP1_T_LEN = l;
    R8_UEP1_CTRL = (R8_UEP1_CTRL & ~MASK_UEP_T_RES) | UEP_T_RES_ACK;
}

/*********************************************************************
 * @fn      DevEP2_IN_Deal
 *
 * @brief   端点2数据上传
 *
 * @param   l   - 上传数据长度(<64B)
 *
 * @return  none
 */
void DevEP2_IN_Deal(uint8_t l)
{
    R8_UEP2_T_LEN = l;
    R8_UEP2_CTRL = (R8_UEP2_CTRL & ~MASK_UEP_T_RES) | UEP_T_RES_ACK;
}

/*********************************************************************
 * @fn      DevEP3_IN_Deal
 *
 * @brief   端点3数据上传
 *
 * @param   l   - 上传数据长度(<64B)
 *
 * @return  none
 */
void DevEP3_IN_Deal(uint8_t l)
{
    R8_UEP3_T_LEN = l;
    R8_UEP3_CTRL = (R8_UEP3_CTRL & ~MASK_UEP_T_RES) | UEP_T_RES_ACK;
}

/*********************************************************************
 * @fn      DevEP4_IN_Deal
 *
 * @brief   端点4数据上传
 *
 * @param   l   - 上传数据长度(<64B)
 *
 * @return  none
 */
void DevEP4_IN_Deal(uint8_t l)
{
    R8_UEP4_T_LEN = l;
    R8_UEP4_CTRL = (R8_UEP4_CTRL & ~MASK_UEP_T_RES) | UEP_T_RES_ACK;
}

/*********************************************************************
 * @fn      USB_DeviceDisable
 *
 * @brief   USB设备功能关闭，进入低功耗前调用。
 *          - 清除 USB 系统控制位 (MASK_UC_SYS_CTRL = 00)，禁用设备并关闭内部上拉电阻
 *          - 禁用 USB 物理端口 I/O
 *          - 注意：不操作 RB_UC_CLR_ALL (不清FIFO)，保留已配置的端点信息
 *
 * @param   none
 *
 * @return  none
 */
void USB_DeviceDisable(void)
{
    // 1. 清除 USB 系统控制位 (bUC_HOST_MODE & bUC_SYS_CTRL1 & bUC_SYS_CTRL0)
    //    MASK_UC_SYS_CTRL = 0x30，清零后为 0 00，即：
    //    "disable USB device and disable internal pullup resistance"
    R8_USB_CTRL &= ~MASK_UC_SYS_CTRL;   // 清除 bit5-4

    // 2. 禁用 USB 物理端口 I/O (RB_UD_PORT_EN = 0)
    //    确保物理层也彻底关闭，进一步降低功耗
    R8_UDEV_CTRL &= ~RB_UD_PORT_EN;
}
