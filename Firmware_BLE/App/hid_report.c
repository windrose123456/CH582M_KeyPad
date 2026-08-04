/*
 * @file hid_report.c
 * @author 
 * @brief 
 * @version V0.1
 * @date 2026-08-04
 * 
 * @copyright Copyright (c) 2026
 * 
 * 提供与传输无关的HID报告发送接口
 * 根据USB就绪状态自动路由到USB或BLE
 * 统一处理输入设备 (EC11旋转编码器、EC11按键、键盘按键)
 * 管理输入状态 (last_key_tick更新、唤醒逻辑)
 *
 */

#include "hid_report.h"
#include <stdio.h>
#include "usb_hid.h"
#include "ble_hid.h"
#include "keypad.h"
#include "ec11.h"

#define KEY_SCAN_PERIODIC_EVT   0x0001
#define EC11_SCAN_PERIODIC_EVT  0x0002

// HID报告任务 ID
uint8_t hidReportTaskId = INVALID_TASK_ID;   

/* ======================== Initialization ======================== */

uint16_t HidReport_ProcessEvent(uint8_t task_id, uint16_t events)
{
    if (events & SYS_EVENT_MSG)
    {
        uint8_t *pMsg;
        if ((pMsg = tmos_msg_receive(hidReportTaskId)) != NULL)
        {
            // 这里可以处理消息，如果没有消息要处理，可以释放掉
            tmos_msg_deallocate(pMsg);
        }
        return (events ^ SYS_EVENT_MSG);
    }

    // 按键定时扫描事件
    if (events & KEY_SCAN_PERIODIC_EVT)
    {
        KeyPad_Scan();
        //printf("KeyPad_Scan()\n");
        static uint16_t last_bitmap = 0;
        uint16_t current_bitmap = KeyPad_GetBitmap();
        // 键值发生变化
        if (current_bitmap != last_bitmap)
        {
            //last_key_tick = TMOS_GetSystemClock();
            printf("current_bitmap: %d\n", current_bitmap);
            HID_SendKeyboardReport(current_bitmap);
            
            last_bitmap = current_bitmap;
        }

        // 再次扫描（自我循环）
        tmos_start_task(hidReportTaskId, KEY_SCAN_PERIODIC_EVT, 5);
        return (events ^ KEY_SCAN_PERIODIC_EVT);
    }

    // EC11定时扫描事件
    if (events & EC11_SCAN_PERIODIC_EVT)
    {
        // EC11旋钮
        int16_t step = EC11_GetStep();         
        if (step != 0) {
            //last_key_tick = TMOS_GetSystemClock();
            static uint32_t EC11_send_tick = 0;
            if (EC11_send_tick == 0)
            {
                HID_SendConsumerReport((step > 0) ? 0x01 : 0x02);
                EC11_send_tick = TMOS_GetSystemClock();
                printf("Volume change send\n");
            }
            if (TMOS_GetSystemClock() - EC11_send_tick > 30)
            {
                // 发送完按键后，必须发送一个空报告(0x00, 0x00)来表示按键已释放[reference:19][reference:20]
                HID_SendConsumerReport(0x00);
                EC11_send_tick = 0;
                EC11_ResetStep();
            }
        }

        // ----- 处理EC11按键事件 -----
        if (EC11_GetKeyState()) {
            //last_key_tick = TMOS_GetSystemClock();

            static uint32_t EC11_send_tick = 0;
            if (EC11_send_tick == 0)
            {
                HID_SendConsumerReport(0x04);
                EC11_send_tick = TMOS_GetSystemClock();
                printf("Mute Toggle\n");
            }
            if (TMOS_GetSystemClock() - EC11_send_tick > 30)
            {
                HID_SendConsumerReport(0x00);
                EC11_send_tick = 0;
                EC11_ResetKey();
            }
        }

        // 再次扫描（自我循环）
        tmos_start_task(hidReportTaskId, EC11_SCAN_PERIODIC_EVT, 20);
        return (events ^ EC11_SCAN_PERIODIC_EVT);
    }

    return 0;
}

void HID_Init(void) {
    KeyPad_Init();
    EC11_Init();
    USB_HID_Init();
    BLE_HID_Init();
    hidReportTaskId = TMOS_ProcessEventRegister(HidReport_ProcessEvent);
    tmos_set_event(hidReportTaskId, KEY_SCAN_PERIODIC_EVT);
    tmos_set_event(hidReportTaskId, EC11_SCAN_PERIODIC_EVT);
}

/* ======================== Report Sending ======================== */

void HID_SendKeyboardReport(uint16_t bitmap) {
    if (USB_HID_IsReady()) {
        /* USB mode: report ID=0x01, 2-byte bitmap */
        uint8_t report[3];
        report[0] = 0x01;
        report[1] = bitmap & 0xFF;
        report[2] = (bitmap >> 8) & 0xFF;
        USB_HID_SendReport(report, 3);
    } else {
        /* BLE mode: direct bitmap, no report ID in payload */
        BLE_HID_SendKbdReport(bitmap);
    }
}

void HID_SendConsumerReport(uint8_t value) {
    if (USB_HID_IsReady()) {
        uint8_t consumerReport[2];
        consumerReport[0] = 0x02;
        consumerReport[1] = value;
        USB_HID_SendReport(consumerReport, 2);
    }
    /* BLE consumer control: extend here if BLE supports it */
}


