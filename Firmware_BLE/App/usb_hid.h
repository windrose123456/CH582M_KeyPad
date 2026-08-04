#ifndef __USB_HID_H__
#define __USB_HID_H__

#include "CONFIG.h"

/* ======================== Public API ======================== */

/**
 * @brief  Initialize USB HID subsystem.
 *         Sets up endpoint buffers, calls USB_DeviceInit(), enables USB IRQ.
 */
void USB_HID_Init(void);

/**
 * @brief  Send a HID report via USB EP1 IN.
 * @param  data  Pointer to report data.
 * @param  len   Report length (clamped to DevEP1SIZE).
 */
void USB_HID_SendReport(uint8_t *data, uint8_t len);

/**
 * @brief  Check if USB is ready (enumerated and not suspended).
 * @return 1 if ready, 0 otherwise.
 */
uint8_t USB_HID_IsReady(void);

/**
 * @brief  Clear USB ready flag (e.g. on suspend).
 */
void USB_HID_ClearReady(void);

#endif /* __USB_HID_H__ */