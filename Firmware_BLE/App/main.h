#ifndef __MAIN_H__
#define __MAIN_H__

#define SLEEP_TIMEOUT_MS 8000 // 960000 是 10min

extern volatile uint8_t enter_sleep_flag; // 进入sleep mode 置1，唤醒中断置0
extern volatile uint8_t wakeup_source;
extern volatile uint32_t last_key_tick; // 低功耗模式使用

/* ======================== 外部接口 ======================== */
void DevWakeup(void);
void Sleep_WakeupConfig(void);
void Enter_SleepMode(void);
void Wakeup_Reinit(void);

#endif