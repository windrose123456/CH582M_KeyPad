#ifndef __SLEEP_H__
#define __SLEEP_H__

#include "CONFIG.h"

extern volatile uint32_t last_send_date_tick; // 低功耗模式使用

/* ======================== 外部接口 ======================== */
void Sleep_EnterCheck(void);

#endif