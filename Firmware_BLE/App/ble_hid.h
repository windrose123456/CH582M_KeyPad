#ifndef __BLE_HID_H__
#define __BLE_HID_H__

#include "CONFIG.h"

/* ======================== TMOS Task Events ======================== */
#define START_DEVICE_EVT          0x0001
#define START_REPORT_EVT          0x0002
#define START_PARAM_UPDATE_EVT    0x0004
#define START_PHY_UPDATE_EVT      0x0008

/* ======================== Public API ======================== */

/**
 * @brief  Initialize BLE HID subsystem.
 *         Registers TMOS task, configures GAP, sets up HID service.
 */
void BLE_HID_Init(void);

/**
 * @brief  Send a keyboard bitmap report via BLE HID.
 * @param  bitmap  16-bit key bitmap (bit0=key1, bit1=key2, ...).
 */
void BLE_HID_SendKbdReport(uint16_t bitmap);

/**
 * @brief  BLE HID TMOS event processor.
 *         Must be called from the TMOS event dispatcher.
 * @param  task_id  TMOS task ID.
 * @param  events   Event flags.
 * @return Unprocessed events.
 */
uint16_t BLE_HID_ProcessEvent(uint8_t task_id, uint16_t events);

#endif /* __BLE_HID_H__ */