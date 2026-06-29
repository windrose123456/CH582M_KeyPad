#ifndef __KEYMAP_H__
#define __KEYMAP_H__

#include "CH58x_common.h"

#define KEYMAP_SIZE    10   // 物理按键总数

/**
 * @brief  根据物理按键序号获取对应的 HID 键盘 Usage ID
 * @param  index  物理按键序号，范围 0 ~ (KEYMAP_SIZE-1)
 * @return 对应的 HID 键码，若 index 无效则返回 0x00
 */
uint8_t KeyMap_GetCode(uint8_t index);

#endif