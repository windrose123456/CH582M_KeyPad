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
#include "main.h"
#include "ws2812b.h"
#include "battery.h"
#include "ring_buffer.h"

volatile uint32_t last_key_tick = 0; // 低功耗模式使用
volatile uint8_t enter_sleep_flag = 0; // 进入sleep mode 置1，唤醒中断置0
volatile uint8_t wakeup_source = 0;

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
        // 其实休眠更适合作为一个 tmos 任务，开定时器，超时直接进入tmos，运行完睡眠
        // 或者 使用 TMOS 的 HAL_SLEEP，如果也是使用sleep模式，就修改成使用 TMOS 自动管理
        if (TMOS_GetSystemClock() - last_key_tick > SLEEP_TIMEOUT_MS) {  // 查询一下TMOS的低功耗管理 HAL_SLEEP，看一下任务调度
            printf("Entering sleep mode...");
            char buf[80];
            int len = snprintf(buf, sizeof(buf), "last_key_tick: %u\r\n", (unsigned int)last_key_tick);
            UART0_SendString((uint8_t *)buf, len);
            len = snprintf(buf, sizeof(buf), "TMOS_GetSystemClock: %u, %u\r\n", 
               (unsigned int)TMOS_GetSystemClock(), (unsigned int)last_key_tick);
            UART0_SendString((uint8_t *)buf, len);
            Enter_SleepMode();           // 阻塞在这里，直到唤醒
            Wakeup_Reinit();             // 唤醒后执行
            last_key_tick = TMOS_GetSystemClock();
            printf("Wakeup complete, wakeup_source = %d\n", wakeup_source);
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
    FP_Init();
    // Battery_Init();
    // WS2812B_Init(); // 灯控有问题，不要开启

    // uint8_t uart3_test_data[] = "2222222222222";
    // UART3_SendString(uart3_test_data, sizeof(uart3_test_data));

    

    // 还需要看门狗

    // 后续将 hidEmuTaskId 任务进行拆分，将按键扫描等新建到一个TMOS任务，在新任务中使用tmos_set_event设置发送标志等

    last_key_tick = TMOS_GetSystemClock();
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
    // ① 关闭 USB（可选，看功耗要求）
    // 如果需要 USB 远程唤醒则保留 USB，否则关闭
    USB_DeviceDisable(); // 同时需要控制 sleep 模式USB供电

    // ② 关闭其他外设时钟
    // 关闭 UART、SPI、定时器等
    // R8_CLK_SYS_CFG = 0x00;  // 具体值参考手册

    // ③ 配置唤醒源
    Sleep_WakeupConfig();

    enter_sleep_flag = 1;

    // ④ 进入待机
    // 进入睡眠，只保留必要的 RAM
    LowPower_Sleep(RB_PWR_RAM30K | RB_PWR_RAM2K);
    // 保留 USB/BLE 单元，以实现快速 USB 唤醒，这个好像没啥用
    // LowPower_Sleep(RB_PWR_RAM30K | RB_PWR_RAM2K | RB_PWR_EXTEND);

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
    // 重新初始化蓝牙，待判断哪些函数需要重新执行，实现蓝牙功能再修改
    CH58X_BLEInit();
    HAL_Init();
    GAPRole_PeripheralInit();
    HidDev_Init();
    HidEmu_Init();

    // ③ 重新初始化 USB
    HID_InitUSBBuffer();
    USB_DeviceInit();
    PFIC_EnableIRQ(USB_IRQn);       //启用中断向量
    mDelaymS(100);
    // 注意：主机可能需要几秒重新枚举
    // 这期间不能发送 HID 报告

    // ④ 重新初始化 GPIO 为正常工作模式
    // 按键矩阵回到扫描模式
    KeyPad_Init();

    // ⑤ 重新初始化 EC11 编码器
    EC11_Init();
}

uint8_t touch_irq_flag = 0;

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

    if (GPIOA_ReadITFlagBit(TOUCH_IRQ_PIN))
    {
        GPIOA_ClearITFlagBit(TOUCH_IRQ_PIN);
        // 发送指纹模块采集指令
        touch_irq_flag += 1;
        //printf("touch_irq\n");

    }
}

/**
 * @brief 将二进制数据转换为十六进制 ASCII 字符串（空格分隔）
 * @param data     二进制数据指针
 * @param len      数据长度
 * @param out_buf  输出缓冲区（需足够大，至少 len*3 字节）
 * @param out_len  输出字符串长度（不含 '\0'）
 */
void hex_to_ascii(const uint8_t *data, uint16_t len, char *out_buf, uint16_t *out_len) {
    uint16_t idx = 0;
    for (uint16_t i = 0; i < len; i++) {
        // 每个字节转两个十六进制字符，占 2 字节
        sprintf(&out_buf[idx], "%02X", data[i]);
        idx += 2;
        // 字节之间加空格（最后一个不加）
        if (i < len - 1) {
            out_buf[idx++] = ' ';
        }
    }
    out_buf[idx] = '\0';   // 字符串结束符
    *out_len = idx;
}

/**
 * @brief 将二进制数据转换为十六进制 ASCII 字符串并通过 UART0 输出
 * @param data     二进制数据指针
 * @param len      数据长度
 * @param prefix   可选前缀字符串（例如 "TX: " 或 "RX: "），可为 NULL
 */
void debug_hex_dump(const uint8_t *data, uint16_t len, const char *prefix) {
    // 缓冲区大小：每个字节占 3 个字符（2个HEX + 1个空格），最后加 '\0'
    // 限制最大打印长度，防止缓冲区溢出
    #define DEBUG_MAX_LEN   128
    char debug_str[DEBUG_MAX_LEN * 3 + 1];
    uint16_t idx = 0;

    // 如果有前缀，先发送前缀
    if (prefix != NULL) {
        UART0_SendString((uint8_t *)prefix, strlen(prefix));
    }

    // 实际打印长度限制，防止缓冲区溢出
    uint16_t print_len = (len > DEBUG_MAX_LEN) ? DEBUG_MAX_LEN : len;

    // 转换每个字节
    for (uint16_t i = 0; i < print_len; i++) {
        // 每个字节转两个十六进制字符
        debug_str[idx++] = "0123456789ABCDEF"[(data[i] >> 4) & 0x0F];
        debug_str[idx++] = "0123456789ABCDEF"[data[i] & 0x0F];
        // 字节之间加空格（最后一个不加）
        if (i < print_len - 1) {
            debug_str[idx++] = ' ';
        }
    }
    debug_str[idx] = '\0';   // 字符串结束符

    // 通过 UART0 发送
    UART0_SendString((uint8_t *)debug_str, idx);

    // 如果数据被截断，加 "..."
    if (len > DEBUG_MAX_LEN) {
        UART0_SendString((uint8_t *)" ...", 4);
    }

    // 发送换行
    UART0_SendString((uint8_t *)"\r\n", 2);
}


