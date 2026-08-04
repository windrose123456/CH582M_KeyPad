#ifndef __HID_REPORT_H__
#define __HID_REPORT_H__

#include "CONFIG.h"

/* ======================== Public API ======================== */

/**
 * @brief  Initialize both USB and BLE HID subsystems.
 *         Calls USB_HID_Init() and BLE_HID_Init() internally.
 */
void HID_Init(void);

/**
 * @brief  Send a keyboard report via the active transport (USB or BLE).
 *         Automatically selects USB if ready, otherwise BLE.
 * @param  bitmap  16-bit key bitmap.
 */
void HID_SendKeyboardReport(uint16_t bitmap);

/**
 * @brief  Send a consumer control report via the active transport.
 * @param  data  Report data pointer.
 * @param  len   Report length.
 */
void HID_SendConsumerReport(uint8_t value);

/**
 * @brief  Process all input devices (EC11, keypad).
 *         Scans inputs, detects changes, sends HID reports.
 *         Called from BLE TMOS task (START_KEYSCAN_EVT).
 *
 *  NOTE: This function will be moved to a dedicated input TMOS task
 *        in a future refactoring step. For now it lives here so that
 *        both USB and BLE paths can reuse the same input logic.
 */
void HID_ProcessInputs(void);

#endif /* __HID_REPORT_H__ */