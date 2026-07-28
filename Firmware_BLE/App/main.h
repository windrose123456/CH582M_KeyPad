#ifndef __MAIN_H__
#define __MAIN_H__

#define SLEEP_TIMEOUT_MS 960000 // 960000 是 10min

extern volatile uint8_t enter_sleep_flag; // 进入sleep mode 置1，唤醒中断置0

/* ======================== 外部接口 ======================== */
void DevWakeup(void);
void Sleep_WakeupConfig(void);
void Enter_SleepMode(void);
void Wakeup_Reinit(void);

#endif