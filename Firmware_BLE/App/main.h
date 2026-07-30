#ifndef __MAIN_H__
#define __MAIN_H__

#define SLEEP_TIMEOUT_MS 960000 // 960000 是 10min，8000 是 用于5s测试

extern volatile uint8_t enter_sleep_flag; // 进入sleep mode 置1，唤醒中断置0
extern volatile uint8_t wakeup_source;
extern volatile uint32_t last_key_tick; // 低功耗模式使用
void hex_to_ascii(const uint8_t *data, uint16_t len, char *out_buf, uint16_t *out_len);
void debug_hex_dump(const uint8_t *data, uint16_t len, const char *prefix);

/* ======================== 外部接口 ======================== */
void DevWakeup(void);
void Sleep_WakeupConfig(void);
void Enter_SleepMode(void);
void Wakeup_Reinit(void);

#endif