#ifndef __HID_REPORT_H__
#define __HID_REPORT_H__

#include "CH58x_common.h"

// 支持的最大接口数量
#define USB_INTERFACE_MAX_NUM       1
// 接口号的最大值，例程只有一个接口，接口号为0
#define USB_INTERFACE_MAX_INDEX      0

#define DevEP0SIZE    0x40
#define DevEP1SIZE    0x40

void DevHIDReport(uint8_t *pdata, uint8_t len);
uint8_t HID_IsReady(void);
void Clear_Ready(void);
void HID_InitUSBBuffer(void);

#endif